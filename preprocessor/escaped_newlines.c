#include <stdlib.h>
#include "escaped_newlines.h"
#include "debug/malloc.h"

struct sstr rm_escaped_newlines(struct sstr input) {
    if (input.n_chars < 2) return input;
    unsigned char *output_chars = MALLOC(input.n_chars);
    const unsigned char *reader = input.chars;
    unsigned char *writer = output_chars;
    // The condition is UB if input.n_chars < 2, but the function should've returned before in that case
    while (reader <= input.chars + input.n_chars - 2) {
        if (reader[0] == '\\' && reader[1] == '\n') {
            reader += 2;
        }
        else {
            *writer = *reader;
            reader++; writer++;
        }
    }
    for (; reader != input.chars + input.n_chars; writer++, reader++) {
        *writer = *reader;
    }
    return (struct sstr){ .chars = output_chars, .n_chars = (size_t)(writer-output_chars) };
}
