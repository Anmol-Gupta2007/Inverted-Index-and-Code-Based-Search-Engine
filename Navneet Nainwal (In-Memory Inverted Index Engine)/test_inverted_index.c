/*
 * test_inverted_index.c -- standalone tests for Member 2's inverted index.
 *
 * Build with -DINDEX_TEST_HOOKS (the Makefile does) so allocation failures can
 * be injected and live allocations counted; the last check asserts that every
 * allocation made by the whole run has been freed.
 */
#include "inverted_index.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks   = 0;
static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            printf("    FAIL  %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                    \
    } while (0)

/* Verifies invariants I1-I5 and the bookkeeping counters of the whole index. */
static void check_invariants(const Index *ix)
{
    size_t b, vocab = 0, postings = 0;
    for (b = 0; b < ix->bucket_count; b++) {
        const VocabNode *v;
        for (v = ix->buckets[b]; v; v = v->next) {
            const PostingNode *p, *last = NULL;
            int prev_doc = -1, df = 0;
            vocab++;
            CHECK(v->hash == djb2_hash(v->word));
            CHECK(v->hash % ix->bucket_count == b);
            CHECK(v->postings != NULL && v->postings_tail != NULL);     /* I5 */
            for (p = v->postings; p; p = p->next) {
                CHECK(p->doc_id > prev_doc);                            /* I1 */
                CHECK(p->term_frequency >= 1);                          /* I2 */
                prev_doc = p->doc_id;
                last = p;
                df++;
            }
            CHECK(df == v->doc_freq);                                   /* I3 */
            CHECK(last == v->postings_tail);                            /* I4 */
            postings += (size_t)df;
        }
    }
    CHECK(vocab == ix->vocab_size);
    CHECK(postings == ix->total_postings);
}

/* Compares a term's posting list with expected ids / tfs. */
static int postings_equal(const Index *ix, const char *term, const int *ids,
                          const int *tfs, size_t n)
{
    const VocabNode *v = find_vocab_node(ix, term);
    const PostingNode *p;
    size_t i = 0;
    if (!v) return n == 0;
    for (p = v->postings; p && i < n; p = p->next, i++) {
        if (p->doc_id != ids[i] || p->term_frequency != tfs[i]) return 0;
    }
    return p == NULL && i == n;
}

/* -------------------------------------------------------------------------- */

static void test_djb2(void)
{
    puts("[1] djb2 hash");
    CHECK(djb2_hash("") == 5381u);
    CHECK(djb2_hash("a") == 177670u);          /* 5381*33 + 97            */
    CHECK(djb2_hash("ab") == 5863208u);        /* 177670*33 + 98          */
    CHECK(djb2_hash("hello") == 261238937u);   /* well-known djb2 value   */
}

static void test_lifecycle_and_invalid(void)
{
    Index *ix;
    puts("[2] lifecycle and invalid arguments");
    free_index(NULL);                                    /* must not crash */

    ix = create_index(0);
    CHECK(ix != NULL);
    CHECK(ix->bucket_count == (size_t)TABLE_SIZE);
    CHECK(ix->vocab_size == 0 && ix->total_postings == 0);
    CHECK(find_vocab_node(ix, "anything") == NULL);

    CHECK(add_token(NULL, "x", 1) == INDEX_ERR_INVALID);
    CHECK(add_token(ix, NULL, 1) == INDEX_ERR_INVALID);
    CHECK(add_token(ix, "", 1) == INDEX_ERR_INVALID);
    CHECK(add_token(ix, "x", -1) == INDEX_ERR_INVALID);
    CHECK(index_add_posting(ix, "x", 1, 0) == INDEX_ERR_INVALID);
    CHECK(find_vocab_node(NULL, "x") == NULL);
    CHECK(find_vocab_node(ix, NULL) == NULL);
    CHECK(ix->vocab_size == 0);
    check_invariants(ix);
    free_index(ix);
}

static void test_basic_insertion(void)
{
    Index *ix = create_index(16);
    const VocabNode *v;
    puts("[3] basic insertion, tf and df");

    CHECK(add_token(ix, "apple", 1) == INDEX_OK);
    CHECK(add_token(ix, "apple", 1) == INDEX_OK);        /* same doc: tf++  */
    CHECK(add_token(ix, "apple", 1) == INDEX_OK);
    CHECK(add_token(ix, "apple", 4) == INDEX_OK);        /* new doc: append */
    CHECK(add_token(ix, "apple", 7) == INDEX_OK);
    CHECK(add_token(ix, "banana", 4) == INDEX_OK);

    CHECK(ix->vocab_size == 2);
    CHECK(ix->total_postings == 4);

    {
        const int ids[] = {1, 4, 7}, tfs[] = {3, 1, 1};
        CHECK(postings_equal(ix, "apple", ids, tfs, 3));
    }
    v = find_vocab_node(ix, "apple");
    CHECK(v && v->doc_freq == 3);
    CHECK(find_vocab_node(ix, "cherry") == NULL);
    CHECK(find_vocab_node(ix, "APPLE") == NULL);         /* case-sensitive  */
    CHECK(find_vocab_node(ix, "appl") == NULL);          /* prefix != match */
    CHECK(find_vocab_node(ix, "apples") == NULL);

    check_invariants(ix);
    free_index(ix);
}

static void test_out_of_order(void)
{
    Index *ix = create_index(8);
    const int ids[] = {1, 2, 3, 5, 9, 10}, tfs[] = {1, 1, 2, 1, 1, 1};
    const int docs[] = {5, 1, 3, 3, 9, 2, 10};           /* 10 = append after */
    size_t i;
    puts("[4] out-of-order doc_ids still give a sorted, unique list");

    for (i = 0; i < sizeof docs / sizeof docs[0]; i++) {
        CHECK(add_token(ix, "term", docs[i]) == INDEX_OK);
    }
    CHECK(postings_equal(ix, "term", ids, tfs, 6));
    CHECK(find_vocab_node(ix, "term")->postings_tail->doc_id == 10);
    check_invariants(ix);
    free_index(ix);
}

static void test_collisions(void)
{
    enum { N = 200 };
    Index *ix = create_index(1);                         /* everything collides */
    IndexStats st;
    char word[16];
    int i;
    puts("[5] single bucket: separate chaining + move-to-front");

    for (i = 0; i < N; i++) {
        sprintf(word, "w%d", i);
        CHECK(add_token(ix, word, i % 7) == INDEX_OK);
    }
    /* Re-insert in a different order: exercises move-to-front on every hit. */
    for (i = N - 1; i >= 0; i--) {
        sprintf(word, "w%d", i);
        CHECK(add_token(ix, word, 100) == INDEX_OK);
    }
    for (i = 0; i < N; i++) {
        const VocabNode *v;
        sprintf(word, "w%d", i);
        v = find_vocab_node(ix, word);
        CHECK(v != NULL);
        if (v) {
            CHECK(strcmp(v->word, word) == 0);
            CHECK(v->doc_freq == 2);          /* doc i%7 and doc 100 */
        }
    }
    CHECK(ix->vocab_size == (size_t)N);
    index_get_stats(ix, &st);
    CHECK(st.used_buckets == 1 && st.longest_chain == (size_t)N);
    CHECK(st.load_factor == (double)N);
    check_invariants(ix);
    free_index(ix);
}

static void test_saturation(void)
{
    Index *ix = create_index(4);
    puts("[6] term_frequency saturates instead of overflowing");
    CHECK(index_add_posting(ix, "x", 1, INT_MAX - 1) == INDEX_OK);
    CHECK(add_token(ix, "x", 1) == INDEX_OK);
    CHECK(find_vocab_node(ix, "x")->postings->term_frequency == INT_MAX);
    CHECK(add_token(ix, "x", 1) == INDEX_OK);
    CHECK(find_vocab_node(ix, "x")->postings->term_frequency == INT_MAX);
    free_index(ix);
}

static void test_allocation_failures(void)
{
#ifdef INDEX_TEST_HOOKS
    Index *ix;
    long live_before, n;
    size_t vs, tp;
    puts("[7] allocation-failure injection (strong guarantee, no leaks)");

    /* create_index needs 2 allocations. */
    for (n = 0; n < 2; n++) {
        live_before = index_test_live_allocs();
        index_test_fail_alloc_after(n);
        CHECK(create_index(16) == NULL);
        index_test_fail_alloc_after(-1);
        CHECK(index_test_live_allocs() == live_before);
    }

    ix = create_index(16);
    CHECK(ix != NULL);
    CHECK(add_token(ix, "alpha", 1) == INDEX_OK);

    /* New term needs 2 allocations: fail at the 1st and at the 2nd. */
    for (n = 0; n < 2; n++) {
        live_before = index_test_live_allocs();
        vs = ix->vocab_size; tp = ix->total_postings;
        index_test_fail_alloc_after(n);
        CHECK(add_token(ix, "beta", 2) == INDEX_ERR_NOMEM);
        index_test_fail_alloc_after(-1);
        CHECK(index_test_live_allocs() == live_before);
        CHECK(ix->vocab_size == vs && ix->total_postings == tp);
        CHECK(find_vocab_node(ix, "beta") == NULL);
        check_invariants(ix);
    }

    /* Known term, new doc (append) needs 1 allocation. */
    live_before = index_test_live_allocs();
    index_test_fail_alloc_after(0);
    CHECK(add_token(ix, "alpha", 5) == INDEX_ERR_NOMEM);
    CHECK(add_token(ix, "alpha", 0) == INDEX_ERR_NOMEM);   /* out-of-order path */
    index_test_fail_alloc_after(-1);
    CHECK(index_test_live_allocs() == live_before);
    {
        const int ids[] = {1}, tfs[] = {1};
        CHECK(postings_equal(ix, "alpha", ids, tfs, 1));
    }
    check_invariants(ix);

    /* Same-doc update needs no allocation, so it succeeds even when malloc fails. */
    index_test_fail_alloc_after(0);
    CHECK(add_token(ix, "alpha", 1) == INDEX_OK);
    index_test_fail_alloc_after(-1);
    CHECK(find_vocab_node(ix, "alpha")->postings->term_frequency == 2);

    /* The index is fully usable after the failures. */
    CHECK(add_token(ix, "beta", 2) == INDEX_OK);
    check_invariants(ix);
    free_index(ix);
#else
    puts("[7] skipped (build with -DINDEX_TEST_HOOKS)");
#endif
}

/* Deterministic LCG so the stress test is reproducible everywhere. */
static unsigned long g_seed = 12345UL;
static unsigned rnd(void)
{
    g_seed = g_seed * 1103515245UL + 12345UL;
    return (unsigned)((g_seed >> 16) & 0x7fffu);
}

static void stress(size_t buckets, int shuffled_docs)
{
    enum { WORDS = 3000, DOCS = 200, TOKENS = 120000 };
    int *ref = calloc((size_t)WORDS * DOCS, sizeof *ref);
    Index *ix = create_index(buckets);
    char word[24];
    int t, w, d;
    size_t postings = 0;

    CHECK(ref != NULL && ix != NULL);
    if (!ref || !ix) { free(ref); free_index(ix); return; }

    for (t = 0; t < TOKENS; t++) {
        /* squash to a skewed (Zipf-like) word distribution */
        unsigned r = rnd();
        w = (int)((r * (rnd() % 64u + 1u)) % WORDS);
        d = shuffled_docs ? (int)(rnd() % DOCS) : (t * DOCS) / TOKENS;
        sprintf(word, "term%d", w);
        if (add_token(ix, word, d) != INDEX_OK) { CHECK(0); break; }
        ref[w * DOCS + d]++;
    }

    for (w = 0; w < WORDS; w++) {
        const VocabNode *v;
        const PostingNode *p;
        int expect_df = 0;
        sprintf(word, "term%d", w);
        for (d = 0; d < DOCS; d++) if (ref[w * DOCS + d]) expect_df++;
        v = find_vocab_node(ix, word);
        if (expect_df == 0) { CHECK(v == NULL); continue; }
        CHECK(v != NULL);
        if (!v) continue;
        CHECK(v->doc_freq == expect_df);
        d = 0;
        for (p = v->postings; p; p = p->next) {
            while (d < DOCS && ref[w * DOCS + d] == 0) d++;
            CHECK(d < DOCS && p->doc_id == d);
            CHECK(d < DOCS && p->term_frequency == ref[w * DOCS + d]);
            d++;
        }
        postings += (size_t)expect_df;
    }
    CHECK(ix->total_postings == postings);
    check_invariants(ix);
    free_index(ix);
    free(ref);
}

static void test_stress(void)
{
    puts("[8] stress vs. reference model (120k tokens)");
    stress(TABLE_SIZE, 0);      /* ascending docs: the pipeline's real pattern */
    stress(TABLE_SIZE, 1);      /* random docs: out-of-order slow path         */
    stress(31, 0);              /* heavily loaded table: long chains           */
}

int main(void)
{
    puts("== Member 2: inverted index tests ==");
    test_djb2();
    test_lifecycle_and_invalid();
    test_basic_insertion();
    test_out_of_order();
    test_collisions();
    test_saturation();
    test_allocation_failures();
    test_stress();

#ifdef INDEX_TEST_HOOKS
    printf("\nLive allocations after all tests: %ld (expected 0)\n",
           index_test_live_allocs());
    CHECK(index_test_live_allocs() == 0);
#endif

    printf("%d checks, %d failures\n", g_checks, g_failures);
    puts(g_failures ? "RESULT: FAIL" : "RESULT: PASS");
    return g_failures ? 1 : 0;
}
