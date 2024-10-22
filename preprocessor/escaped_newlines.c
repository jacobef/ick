#include <stdlib.h>
#include "escaped_newlines.h"

#include "debug/malloc.h"

struct escaped_newlines_replacement_info rm_escaped_newlines(const sstr in) {
    size_t_vec backslash_locations = size_t_vec_new(0);
    if (in.len < 2) {
        unsigned char *out_chars = MALLOC(in.len);
        memcpy(out_chars, in.data, in.len);
        return (struct escaped_newlines_replacement_info) {
            .result = { .data = out_chars, .len = in.len },
            .backslash_locations = backslash_locations.arr
        };
    }
    unsigned char *out_chars = MALLOC(in.len);
    size_t in_i = 0;
    size_t out_i = 0;
    // The condition is UB if input.n < 2, but the function should've returned before in that case
    while (in_i <= in.len - 2) {
        if (in.data[in_i] == '\\' && in.data[in_i+1] == '\n') {
            size_t_vec_append(&backslash_locations, in_i);
            in_i += 2;
        }
        else {
            out_chars[out_i] = in.data[in_i];
            in_i++; out_i++;
        }
    }
    for (; in_i < in.len; in_i++, out_i++) {
        out_chars[out_i] = in.data[in_i];
    }
    return (struct escaped_newlines_replacement_info){
        .result = { .data = out_chars, .len = out_i },
        .backslash_locations = backslash_locations.arr
    };
}
