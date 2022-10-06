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

struct chars replace_trigraphs(struct chars input) {

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

    char *output_chars = malloc(input.n_chars);
    const char *reader = input.chars;
    char *writer = output_chars;
    // 3 because that's the length of a trigraph
    // UB if input.n_chars<3 but the function should've returned before in that case
    while(reader <= input.chars + input.n_chars - 3) {
        if (is_trigraph(reader)) {
            // unsigned cast is OK because guaranteed to be nonnegative; see 6.2.5.3
            *writer = trigraphs_to_replacements[(unsigned char)reader[2]];
            reader += 3, writer++;
        }
        else {
            if (writer != reader) *writer = *reader;
            reader++, writer++;
        }
    }
    for (; reader != input.chars + input.n_chars; writer++, reader++) {
        *writer = *reader;
    }
    return (struct chars){.chars = output_chars, .n_chars = writer-output_chars};
}
