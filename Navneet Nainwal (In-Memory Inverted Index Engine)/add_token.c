#include "inverted_index.h"

/* Records one occurrence of word in document doc_id. */
int add_token(Index *index, const char *word, int doc_id)
{
    return index_add_posting(index, word, doc_id, 1);
}
