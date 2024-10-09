#include "trigraphs.h"

#include <string.h>
#include <stdbool.h>
#include "data_structures/vector.h"

static bool is_trigraph(const unsigned char *trigraph) {
    if (trigraph[0] != '?' || trigraph[1] != '?') return false;
    switch (trigraph[2]) {
    case '=': case '/': case '\'': case '(': case ')': case '!': case '<': case '>': case '-':
        return true;
    default:
        return false;
    }
}

struct trigraph_replacement_info replace_trigraphs(const sstr in) {
    size_t_vec original_trigraph_locations = size_t_vec_new(0);
    if (in.len < 3) {
        unsigned char *out_chars = MALLOC(in.len);
        memcpy(out_chars, in.data, in.len);
        return (struct trigraph_replacement_info) {
            .result = { .data = out_chars, .len = in.len },
            .original_trigraph_locations = original_trigraph_locations.arr
        };
    }
    // This can be an unsigned char array because these characters must be non-negative; see 6.2.5 paragraph 3.
    // For the same reason, they're also OK to use as indices.
    const unsigned char trigraphs_to_replacements[] = {
            ['='] = '#',
            ['/'] = '\\',
            ['('] = '[',
            [')'] = ']',
            ['!'] = '|',
            ['<'] = '{',
            ['>'] = '}',
            ['-'] = '~'
    };

    unsigned char *out_chars = MALLOC(in.len);
    size_t in_i = 0;
    size_t out_i = 0;
    // 3 because that's the length of a trigraph
    // UB if in.n <3 but the function should've returned before in that case
    while(in_i <= in.len - 3) {
        if (is_trigraph(&in.data[in_i])) {
            out_chars[out_i] = trigraphs_to_replacements[in.data[in_i + 2]];
            size_t_vec_append(&original_trigraph_locations, in_i);
            in_i += 3; out_i++;
        }
        else {
            out_chars[out_i] = in.data[in_i];
            in_i++; out_i++;
        }
    }
    for (; in_i < in.len; in_i++, out_i++) {
        out_chars[out_i] = in.data[in_i];
    }
    return (struct trigraph_replacement_info){
        .result = {.data = out_chars, .len = out_i},
        .original_trigraph_locations = original_trigraph_locations.arr
    };
}
