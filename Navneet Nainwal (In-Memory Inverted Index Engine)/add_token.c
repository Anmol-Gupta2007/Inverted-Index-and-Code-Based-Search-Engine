#include "inverted_index.h"
int add_token(Index *index, const char *word, int doc_id)
{
    return index_add_posting(index, word, doc_id, 1);
}
