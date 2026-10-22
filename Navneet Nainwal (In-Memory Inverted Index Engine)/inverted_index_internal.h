#ifndef INVERTED_INDEX_INTERNAL_H
#define INVERTED_INDEX_INTERNAL_H
#include <stddef.h>
#include <stdint.h>

#include "inverted_index.h"
uint32_t hash_and_len(const char *s, size_t *len_out);
PostingNode *new_posting(int doc_id, int tf);
int add_posting_existing(Index *index, VocabNode *v, int doc_id, int tf);
int add_new_term(Index *index, VocabNode **head, uint32_t h,
                 const char *word, size_t len, int doc_id, int tf);

#endif
