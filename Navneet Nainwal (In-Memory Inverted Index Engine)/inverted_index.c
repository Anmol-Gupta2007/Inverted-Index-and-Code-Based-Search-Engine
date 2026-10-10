/*
 * inverted_index.c  --  Member 2: in-memory inverted index (pure C11)
 *
 * See inverted_index.h for the invariants, optimisations and ownership rules.
 */
#include "inverted_index.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Allocation layer (instrumented only in test builds)                        */
/* ------------------------------------------------------------------------- */
#ifdef INDEX_TEST_HOOKS
static long g_fail_after = -1;
static long g_live       = 0;

void index_test_fail_alloc_after(long n) { g_fail_after = n; }
long index_test_live_allocs(void)        { return g_live; }

static int should_fail(void)
{
    if (g_fail_after < 0) return 0;
    if (g_fail_after == 0) return 1;      /* stays 0: keeps failing until reset */
    g_fail_after--;
    return 0;
}
static void *idx_malloc(size_t n)
{
    void *p;
    if (should_fail()) return NULL;
    p = malloc(n);
    if (p) g_live++;
    return p;
}
static void *idx_calloc(size_t n, size_t sz)
{
    void *p;
    if (should_fail()) return NULL;
    p = calloc(n, sz);
    if (p) g_live++;
    return p;
}
static void idx_free(void *p)
{
    if (p) { g_live--; free(p); }
}
#else
#define idx_malloc(n)     malloc(n)
#define idx_calloc(n, sz) calloc((n), (sz))
#define idx_free(p)       free(p)
#endif

/* ------------------------------------------------------------------------- */
/* Hashing                                                                    */
/* ------------------------------------------------------------------------- */

/* djb2 and strlen in one pass over the string. */
static uint32_t hash_and_len(const char *s, size_t *len_out)
{
    const unsigned char *p = (const unsigned char *)s;
    uint32_t h = 5381u;
    while (*p) {
        h = ((h << 5) + h) + *p;          /* h * 33 + c, wraps mod 2^32 */
        p++;
    }
    *len_out = (size_t)((const char *)p - s);
    return h;
}

uint32_t djb2_hash(const char *word)
{
    size_t len;
    return word ? hash_and_len(word, &len) : 5381u;
}

/* ------------------------------------------------------------------------- */
/* Lifecycle                                                                  */
/* ------------------------------------------------------------------------- */

Index *create_index(size_t bucket_count)
{
    Index *index;

    if (bucket_count == 0) bucket_count = TABLE_SIZE;
    if (bucket_count > SIZE_MAX / sizeof(VocabNode *)) return NULL;

    index = idx_malloc(sizeof *index);
    if (!index) return NULL;

    /* calloc yields all-bits-zero pointers, i.e. NULL, on every supported
     * platform (GCC/Clang on x86-64, ARM64 incl. Apple Silicon). */
    index->buckets = idx_calloc(bucket_count, sizeof *index->buckets);
    if (!index->buckets) {
        idx_free(index);
        return NULL;
    }
    index->bucket_count   = bucket_count;
    index->vocab_size     = 0;
    index->total_postings = 0;
    return index;
}

void free_index(Index *index)
{
    size_t i;
    if (!index) return;

    if (index->buckets) {
        for (i = 0; i < index->bucket_count; i++) {
            VocabNode *v = index->buckets[i];
            while (v) {
                VocabNode *next_v = v->next;
                PostingNode *p = v->postings;
                while (p) {                       /* iterative: no recursion */
                    PostingNode *next_p = p->next;
                    idx_free(p);
                    p = next_p;
                }
                idx_free(v);                      /* word is inline: one free */
                v = next_v;
            }
        }
    }
    idx_free(index->buckets);
    idx_free(index);
}

/* ------------------------------------------------------------------------- */
/* Insertion                                                                  */
/* ------------------------------------------------------------------------- */

static PostingNode *new_posting(int doc_id, int tf)
{
    PostingNode *n = idx_malloc(sizeof *n);
    if (!n) return NULL;
    n->doc_id         = doc_id;
    n->term_frequency = tf;
    n->next           = NULL;
    return n;
}

/* Saturating add so a pathological count can never overflow int. */
static void add_tf(PostingNode *n, int tf)
{
    n->term_frequency = (n->term_frequency > INT_MAX - tf)
                            ? INT_MAX
                            : n->term_frequency + tf;
}

/* Adds (doc_id, tf) to an existing term. Nothing is modified on failure. */
static int add_posting_existing(Index *index, VocabNode *v, int doc_id, int tf)
{
    PostingNode *tail = v->postings_tail;
    PostingNode *node;

    /* Fast path 1: same document as the most recent posting. */
    if (tail->doc_id == doc_id) {
        add_tf(tail, tf);
        return INDEX_OK;
    }

    /* Fast path 2: new, largest doc_id -> O(1) append at the tail. */
    if (tail->doc_id < doc_id) {
        node = new_posting(doc_id, tf);
        if (!node) return INDEX_ERR_NOMEM;
        tail->next      = node;
        v->postings_tail = node;
    } else {
        /* Slow path: doc_id arrived out of order. Sorted insert keeps I1.
         * Since doc_id < tail->doc_id the scan always stops at or before
         * the tail, so the tail pointer stays valid. */
        PostingNode *prev = NULL;
        PostingNode *cur  = v->postings;
        while (cur && cur->doc_id < doc_id) {
            prev = cur;
            cur  = cur->next;
        }
        if (cur && cur->doc_id == doc_id) {
            add_tf(cur, tf);
            return INDEX_OK;
        }
        node = new_posting(doc_id, tf);
        if (!node) return INDEX_ERR_NOMEM;
        node->next = cur;
        if (prev) prev->next = node; else v->postings = node;
    }

    v->doc_freq++;
    index->total_postings++;
    return INDEX_OK;
}

/* Creates a brand-new term in bucket `*head`. Nothing is modified on failure. */
static int add_new_term(Index *index, VocabNode **head, uint32_t h,
                        const char *word, size_t len, int doc_id, int tf)
{
    VocabNode *v;
    PostingNode *p;

    if (len > SIZE_MAX - sizeof *v - 1) return INDEX_ERR_NOMEM;

    p = new_posting(doc_id, tf);
    if (!p) return INDEX_ERR_NOMEM;

    v = idx_malloc(sizeof *v + len + 1);
    if (!v) {
        idx_free(p);                      /* roll back: no leak on failure */
        return INDEX_ERR_NOMEM;
    }
    memcpy(v->word, word, len + 1);
    v->hash          = h;
    v->doc_freq      = 1;
    v->postings      = p;
    v->postings_tail = p;
    v->next          = *head;             /* insert at chain head: O(1) */
    *head            = v;

    index->vocab_size++;
    index->total_postings++;
    return INDEX_OK;
}

int index_add_posting(Index *index, const char *word, int doc_id, int tf)
{
    size_t len;
    uint32_t h;
    VocabNode **head;
    VocabNode *prev, *v;

    if (!index || !index->buckets || index->bucket_count == 0 ||
        !word || word[0] == '\0' || doc_id < 0 || tf < 1) {
        return INDEX_ERR_INVALID;
    }

    h    = hash_and_len(word, &len);
    head = &index->buckets[h % index->bucket_count];

    prev = NULL;
    for (v = *head; v; prev = v, v = v->next) {
        if (v->hash == h && strcmp(v->word, word) == 0) break;
    }

    if (!v) return add_new_term(index, head, h, word, len, doc_id, tf);

    /* Move-to-front: hot terms get shorter probes next time. Only relinks
     * the chain; no node is moved in memory or freed, and it is safe even if
     * the posting update below fails. */
    if (prev) {
        prev->next = v->next;
        v->next    = *head;
        *head      = v;
    }
    return add_posting_existing(index, v, doc_id, tf);
}

int add_token(Index *index, const char *word, int doc_id)
{
    return index_add_posting(index, word, doc_id, 1);
}

/* ------------------------------------------------------------------------- */
/* Lookup                                                                     */
/* ------------------------------------------------------------------------- */

const VocabNode *find_vocab_node(const Index *index, const char *term)
{
    size_t len;
    uint32_t h;
    const VocabNode *v;

    if (!index || !index->buckets || index->bucket_count == 0 || !term) {
        return NULL;
    }
    h = hash_and_len(term, &len);
    for (v = index->buckets[h % index->bucket_count]; v; v = v->next) {
        if (v->hash == h && strcmp(v->word, term) == 0) return v;
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* Diagnostics                                                                */
/* ------------------------------------------------------------------------- */

void index_get_stats(const Index *index, IndexStats *out)
{
    size_t i;
    if (!index || !out) return;

    out->bucket_count   = index->bucket_count;
    out->used_buckets   = 0;
    out->longest_chain  = 0;
    out->vocab_size     = index->vocab_size;
    out->total_postings = index->total_postings;
    out->load_factor    = index->bucket_count
                              ? (double)index->vocab_size /
                                    (double)index->bucket_count
                              : 0.0;

    for (i = 0; i < index->bucket_count; i++) {
        size_t chain = 0;
        const VocabNode *v;
        for (v = index->buckets[i]; v; v = v->next) chain++;
        if (chain) out->used_buckets++;
        if (chain > out->longest_chain) out->longest_chain = chain;
    }
}
