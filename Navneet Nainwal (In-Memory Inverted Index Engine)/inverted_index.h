#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef TABLE_SIZE
#define TABLE_SIZE 4093
#endif

typedef struct PostingNode {
    int doc_id;
    int term_frequency;
    struct PostingNode *next;
} PostingNode;

typedef struct VocabNode {
    struct VocabNode *next;       /* next term in the same bucket chain      */
    PostingNode *postings;        /* head: smallest doc_id (borrowed by users) */
    PostingNode *postings_tail;   /* last node, for O(1) append               */
    uint32_t hash;                /* cached full djb2 hash of `word`          */
    int doc_freq;                 /* DF(t) == length of the posting list      */
    char word[];                  /* NUL-terminated, stored inline            */
} VocabNode;

typedef struct Index {
    VocabNode **buckets;
    size_t bucket_count;
    size_t vocab_size;            /* number of distinct terms                 */
    size_t total_postings;        /* total PostingNodes across all terms      */
} Index;

/* Return codes. */
enum {
    INDEX_OK          =  0,
    INDEX_ERR_INVALID = -1,       /* NULL/empty argument, doc_id < 0, tf < 1  */
    INDEX_ERR_NOMEM   = -2        /* allocation failed; index left unchanged  */
};

/* ---- Lifecycle ----------------------------------------------------------- */

/* Allocates an empty index with `bucket_count` buckets (0 -> TABLE_SIZE).
 * Returns an OWNED pointer, or NULL on allocation failure. */
Index *create_index(size_t bucket_count);

/* Frees every PostingNode, every VocabNode and the Index itself.
 * Safe on NULL. The caller should not use the pointer afterwards. */
void free_index(Index *index);

/* ---- Insertion ------------------------------------------------------------ */

/* Records one occurrence of `word` in document `doc_id`.
 *   - new term            -> creates a VocabNode (1 malloc) + a PostingNode
 *   - known term, new doc -> appends a PostingNode (O(1) if doc_id is the
 *                            largest seen for that term, else sorted insert)
 *   - known term, same doc-> term_frequency++ (saturates at INT_MAX)
 * `word` must already be normalised (Member 1 lower-cases it); it is copied.
 * Returns INDEX_OK, INDEX_ERR_INVALID or INDEX_ERR_NOMEM. On any error the
 * index is exactly as it was before the call (strong guarantee). */
int add_token(Index *index, const char *word, int doc_id);

/* Same as add_token() but adds `tf` (>= 1) occurrences at once. Useful for
 * bulk loading and for fixtures that already know the counts. */
int index_add_posting(Index *index, const char *word, int doc_id, int tf);

/* ---- Lookup (read-only) ---------------------------------------------------- */

/* Returns the vocabulary entry for `term`, or NULL if absent / invalid
 * arguments. BORROWED pointer, never modifies the index.
 * Expected O(1) after hashing; O(chain length) worst case. */
const VocabNode *find_vocab_node(const Index *index, const char *term);

/* djb2 hash (h = h*33 + c, seed 5381) with 32-bit wrap-around. */
uint32_t djb2_hash(const char *word);

/* ---- Diagnostics ----------------------------------------------------------- */

typedef struct IndexStats {
    size_t bucket_count;
    size_t used_buckets;          /* buckets with at least one term           */
    size_t longest_chain;
    size_t vocab_size;
    size_t total_postings;
    double load_factor;           /* vocab_size / bucket_count                */
} IndexStats;

/* Fills `out` (O(bucket_count + vocab_size)). No-op if either is NULL. */
void index_get_stats(const Index *index, IndexStats *out);

/* ---- Compatibility aliases (names used by Member 3's test fixture) --------- */
#define index_create(n)   create_index(n)
#define index_destroy(ix) free_index(ix)

/* ---- Test hooks: compiled only with -DINDEX_TEST_HOOKS ---------------------- */
#ifdef INDEX_TEST_HOOKS
/* Make the n-th next allocation fail (0 = the very next one); -1 disables. */
void index_test_fail_alloc_after(long n);
/* Number of allocations currently live (allocated minus freed). */
long index_test_live_allocs(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INVERTED_INDEX_H */
