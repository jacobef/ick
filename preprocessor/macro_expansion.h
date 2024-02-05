#ifndef MACROS_H
#define MACROS_H

#include <stdbool.h>
#include "preprocessor/pp_token.h"
#include "data_structures/map.h"

static bool pp_tokens_equal(struct preprocessing_token t1, struct preprocessing_token t2) {
    const unsigned char *c1, *c2;
    for (c1 = t1.first, c2 = t2.first; c1 != t1.end && c2 != t2.end; c1++, c2++) {
        if (*c1 != *c2) return false;
    }
    return c1 == t1.end && c2 == t2.end;
}

static size_t hash_pp_token(struct preprocessing_token token, size_t capacity) {
    size_t chars_sum = 0;
    for (const unsigned char *c = token.first; c != token.end; c++) {
        chars_sum += (size_t)(*c);
    }
    return chars_sum % capacity;
}

DEFINE_MAP_TYPE_AND_FUNCTIONS(pp_token, pp_token_vec, hash_pp_token, pp_tokens_equal)

#endif //MACROS_H
