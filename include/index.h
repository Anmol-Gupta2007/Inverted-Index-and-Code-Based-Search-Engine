#ifndef INDEX_H
#define INDEX_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define TABLE_SIZE 10007 
typedef struct PostingNode {
    int doc_id;
    int term_frequency;
    struct PostingNode *next;
} PostingNode;
typedef struct HashEntry {
    char *word;
    PostingNode *head;
    struct HashEntry *next; 
} HashEntry;

typedef struct InvertedIndex {
    HashEntry *buckets[TABLE_SIZE];
    int total_documents;
} InvertedIndex;

InvertedIndex* create_inverted_index(void);
void free_inverted_index(InvertedIndex *index);

void insert_token(InvertedIndex *index, const char *token, int doc_id);
PostingNode* get_postings(InvertedIndex *index, const char *token);

void print_index_stats(const InvertedIndex *index);

#ifdef __cplusplus
}
#endif
#endif
