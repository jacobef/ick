#include "trigraphs.h"

#include <limits.h>
#include <stdbool.h>
#include "data_structures/vector.h"
#include "driver/diagnostics.h"
#include "lines.h"

static bool is_trigraph(const char *trigraph) {
    if (trigraph[0] != '?' || trigraph[1] != '?') return false;
    switch (trigraph[2]) {
    case '=': case '/': case '\'': case '(': case ')': case '!': case '<': case '>': case '-':
        return true;
    default:
        return false;
    }
}

struct lines replace_trigraphs(struct lines input) {

    size_t n_trigraphs = 0;
    for (size_t i = 0; i < input.n_chars; i++) {
        if (is_trigraph(&input.chars[i])) n_trigraphs++;
    }
    if (n_trigraphs == 0) return input;

    char trigraphs_to_replacements[CHAR_MAX+1];
    trigraphs_to_replacements['='] = '#';
    trigraphs_to_replacements['/'] = '\\';
    trigraphs_to_replacements['\''] = '^';
    trigraphs_to_replacements['('] = '[';
    trigraphs_to_replacements[')'] = ']';
    trigraphs_to_replacements['!'] = '|';
    trigraphs_to_replacements['<'] = '{';
    trigraphs_to_replacements['>'] = '}';
    trigraphs_to_replacements['-'] = '~';

    const size_t output_len = input.n_chars - n_trigraphs * 2; // fsanitize=address doesn't detect when this is too small, kinda scary
    char *output_chars = malloc(output_len);
    const char *reader = input.chars;
    char *writer = output_chars;
    // TODO maybe use trigraph locations thing again or maybe not cause this is more readable
    // 3 because that's the length of a trigraph
    // UB if input.n_chars<3 but the function should've returned before in that case
    while(reader <= input.chars + input.n_chars - 3) {
        if (is_trigraph(reader)) {
            *writer = trigraphs_to_replacements[(unsigned char)reader[2]];
            reader += 3, writer++;
        }
        else {
            *writer = *reader;
            reader++, writer++;
        }
    }
    for (; reader != input.chars + input.n_chars; writer++, reader++) {
        *writer = *reader;
    }
    return get_lines(output_chars, output_len);
}
