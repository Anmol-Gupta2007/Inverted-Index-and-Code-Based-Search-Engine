#include <string.h>

#include "inverted_index.h"
#include "inverted_index_internal.h"
const VocabNode *find_vocab_node(const Index *index,
                                 const char *term)
{
    size_t len;
    uint32_t h;
    const VocabNode *v;

    if (!index ||
        !index->buckets ||
        index->bucket_count == 0 ||
        !term) {

        return NULL;
    }

    h = hash_and_len(term, &len);

    for (v = index->buckets[h % index->bucket_count];
         v;
         v = v->next) {

        if (v->hash == h &&
            strcmp(v->word, term) == 0) {

            return v;
        }
    }

    return NULL;
}
