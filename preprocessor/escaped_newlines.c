#include <stdlib.h>
#include "escaped_newlines.h"

#include <string.h>

#include "debug/malloc.h"

struct escaped_newlines_replacement_info rm_escaped_newlines(const struct str_view in) {
    size_t_vec backslash_locations = size_t_vec_new(0);
    if (in.n < 2) {
        unsigned char *out_chars = MALLOC(in.n);
        memcpy(out_chars, in.chars, in.n);
        return (struct escaped_newlines_replacement_info) {
            .result = { .chars = out_chars, .n = in.n },
            .backslash_locations = backslash_locations
        };
    }
    unsigned char *out_chars = MALLOC(in.n);
    size_t in_i = 0;
    size_t out_i = 0;
    // The condition is UB if input.n < 2, but the function should've returned before in that case
    while (in_i <= in.n - 2) {
        if (in.chars[in_i] == '\\' && in.chars[in_i+1] == '\n') {
            size_t_vec_append(&backslash_locations, in_i);
            in_i += 2;
        }
        else {
            out_chars[out_i] = in.chars[in_i];
            in_i++; out_i++;
        }
    }
    for (; in_i < in.n; in_i++, out_i++) {
        out_chars[out_i] = in.chars[in_i];
    }
    return (struct escaped_newlines_replacement_info){
        .result = { .chars = out_chars, .n = out_i },
        .backslash_locations = backslash_locations
    };
}
