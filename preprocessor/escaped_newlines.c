//
// Created by Jacob Friedman on 9/4/22.
//

#include <stdlib.h>
#include "escaped_newlines.h"

struct lines rm_escaped_newlines(struct lines input) {
    if (input.n_chars < 2) return input;
    char *output_chars = malloc(input.n_chars);
    const char *reader = input.chars;
    char *writer = output_chars;
    // UB if input.n_chars < 2 but function should've returned before in that case
    while (reader <= input.chars + input.n_chars - 2) {
        if (reader[0] == '\\' && reader[1] == '\n') {
            reader += 2;
        }
        else {
            *writer = *reader;
            reader++, writer++;
        }
    }
    for (; reader != input.chars + input.n_chars; writer++, reader++) {
        *writer = *reader;
    }
    return get_lines(output_chars, writer-output_chars);
}
