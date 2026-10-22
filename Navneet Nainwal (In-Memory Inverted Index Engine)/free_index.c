#include <stdlib.h>
#include "inverted_index.h"
void free_index(Index *index)
{
    size_t i;

    if (!index)
        return;

    if (index->buckets) {
        for (i = 0; i < index->bucket_count; i++) {

            VocabNode *v = index->buckets[i];

            while (v) {
                VocabNode *next_v = v->next;
                PostingNode *p = v->postings;

                while (p) {
                    PostingNode *next_p = p->next;
                    free(p);
                    p = next_p;
                }

                free(v);
                v = next_v;
            }
        }
    }

    free(index->buckets);
    free(index);
}
