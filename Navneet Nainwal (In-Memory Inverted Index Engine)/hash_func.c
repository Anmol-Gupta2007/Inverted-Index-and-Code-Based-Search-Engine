#include "inverted_index.h"
#include "inverted_index_internal.h"
uint32_t hash_and_len(const char *s, size_t *len_out)
{
    const unsigned char *p = (const unsigned char *)s;
    uint32_t h = 5381u;
    while (*p) {
        h = ((h << 5) + h) + *p;
        p++;
    }
    *len_out = (size_t)((const char *)p - s);
    return h;
}
