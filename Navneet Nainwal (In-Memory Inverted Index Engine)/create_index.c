#include <stdint.h>
#include <stdlib.h>
#include "inverted_index.h"
Index *create_index(size_t bucket_count)
{
    Index *index;
    if (bucket_count == 0)
        bucket_count = TABLE_SIZE;
    if (bucket_count > SIZE_MAX / sizeof(VocabNode *))
        return NULL;
    index = malloc(sizeof *index);
    if (!index)
        return NULL;
    index->buckets = calloc(bucket_count, sizeof *index->buckets);
    if (!index->buckets) {
        free(index);
        return NULL;
    }
    index->bucket_count   = bucket_count;
    index->vocab_size     = 0;
    index->total_postings = 0;
    return index;
}
