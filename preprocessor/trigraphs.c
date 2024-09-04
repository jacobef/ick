#include "trigraphs.h"

#include <string.h>
#include <stdbool.h>
#include "data_structures/vector.h"
#include "data_structures/sized_str.h"

static bool is_trigraph(const unsigned char *trigraph) {
    if (trigraph[0] != '?' || trigraph[1] != '?') return false;
    switch (trigraph[2]) {
    case '=': case '/': case '\'': case '(': case ')': case '!': case '<': case '>': case '-':
        return true;
    default:
        return false;
    }
}

struct trigraph_replacement_info replace_trigraphs(const struct str_view in) {
    size_t_vec original_trigraph_locations = size_t_vec_new(0);
    if (in.n < 3) {
        unsigned char *out_chars = MALLOC(in.n);
        memcpy(out_chars, in.chars, in.n);
        return (struct trigraph_replacement_info) {
            .result = { .chars = out_chars, .n = in.n },
            .original_trigraph_locations = original_trigraph_locations
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

    unsigned char *out_chars = MALLOC(in.n);
    size_t in_i = 0;
    size_t out_i = 0;
    // 3 because that's the length of a trigraph
    // UB if in.n <3 but the function should've returned before in that case
    while(in_i <= in.n - 3) {
        if (is_trigraph(&in.chars[in_i])) {
            out_chars[out_i] = trigraphs_to_replacements[in.chars[in_i + 2]];
            size_t_vec_append(&original_trigraph_locations, in_i);
            in_i += 3; out_i++;
        }
        else {
            out_chars[out_i] = in.chars[in_i];
            in_i++; out_i++;
        }
    }
    for (; in_i < in.n; in_i++, out_i++) {
        out_chars[out_i] = in.chars[in_i];
    }
    return (struct trigraph_replacement_info){
        .result = {.chars = out_chars, .n = out_i},
        .original_trigraph_locations = original_trigraph_locations
    };
}
