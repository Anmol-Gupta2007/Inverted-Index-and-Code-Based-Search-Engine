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
    struct VocabNode *next;       
    PostingNode *postings;       
    PostingNode *postings_tail;   
    uint32_t hash;                
    int doc_freq;                 
    char word[];                  
} VocabNode;
typedef struct Index {
    VocabNode **buckets;
    size_t bucket_count;
    size_t vocab_size;            
    size_t total_postings;        
} Index;
enum {
    INDEX_OK          =  0,
    INDEX_ERR_INVALID = -1,       
    INDEX_ERR_NOMEM   = -2       
};
Index *create_index(size_t bucket_count);
void free_index(Index *index);
int add_token(Index *index, const char *word, int doc_id);
int index_add_posting(Index *index, const char *word, int doc_id, int tf);
const VocabNode *find_vocab_node(const Index *index, const char *term);


uint32_t djb2_hash(const char *word);

typedef struct IndexStats {
    size_t bucket_count;
    size_t used_buckets;         
    size_t longest_chain;
    size_t vocab_size;
    size_t total_postings;
    double load_factor;           
} IndexStats;
void index_get_stats(const Index *index, IndexStats *out);
#define index_create(n)   create_index(n)
#define index_destroy(ix) free_index(ix)
#ifdef INDEX_TEST_HOOKS
void index_test_fail_alloc_after(long n);
long index_test_live_allocs(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
