#include "trigraphs.h"

#include <limits.h>
#include <stdbool.h>
#include "data_structures/vector.h"
#include "sized_str.h"

static bool is_trigraph(const unsigned char *trigraph) {
    if (trigraph[0] != '?' || trigraph[1] != '?') return false;
    switch (trigraph[2]) {
    case '=': case '/': case '\'': case '(': case ')': case '!': case '<': case '>': case '-':
        return true;
    default:
        return false;
    }
}

struct sstr replace_trigraphs(struct sstr input) {
    // This can be an unsigned char array because these characters must be nonnegative; see 6.2.5 paragraph 3.
    // For the same reason, they're also OK to use as indices.
    unsigned char trigraphs_to_replacements[CHAR_MAX+1];
    trigraphs_to_replacements['='] = '#';
    trigraphs_to_replacements['/'] = '\\';
    trigraphs_to_replacements['\''] = '^';
    trigraphs_to_replacements['('] = '[';
    trigraphs_to_replacements[')'] = ']';
    trigraphs_to_replacements['!'] = '|';
    trigraphs_to_replacements['<'] = '{';
    trigraphs_to_replacements['>'] = '}';
    trigraphs_to_replacements['-'] = '~';

    unsigned char *output_chars = MALLOC(input.n);
    const unsigned char *reader = input.chars;
    unsigned char *writer = output_chars;
    // 3 because that's the length of a trigraph
    // UB if input.n_chars<3 but the function should've returned before in that case
    while(reader <= input.chars + input.n - 3) {
        if (is_trigraph(reader)) {
            *writer = trigraphs_to_replacements[reader[2]];
            reader += 3; writer++;
        }
        else {
            if (writer != reader) *writer = *reader;
            reader++; writer++;
        }
    }
    for (; reader != input.chars + input.n; writer++, reader++) {
        *writer = *reader;
    }
    return (struct sstr){.chars = output_chars, .n = (size_t)(writer - output_chars)};
}
