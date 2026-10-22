#include <string.h>

#include "inverted_index.h"
#include "inverted_index_internal.h"
int index_add_posting(Index *index,
                      const char *word,
                      int doc_id,
                      int tf)
{
    size_t len;
    uint32_t h;
    VocabNode **head;
    VocabNode *prev;
    VocabNode *v;

    if (!index ||
        !index->buckets ||
        index->bucket_count == 0 ||
        !word ||
        word[0] == '\0' ||
        doc_id < 0 ||
        tf < 1) {

        return INDEX_ERR_INVALID;
    }

    h = hash_and_len(word, &len);

    head = &index->buckets[h % index->bucket_count];

    prev = NULL;

    for (v = *head; v; prev = v, v = v->next) {

        if (v->hash == h &&
            strcmp(v->word, word) == 0) {

            break;
        }
    }

    if (!v)
        return add_new_term(index, head, h, word, len, doc_id, tf);
    if (prev) {
        prev->next = v->next;
        v->next = *head;
        *head = v;
    }

    return add_posting_existing(index, v, doc_id, tf);
}
