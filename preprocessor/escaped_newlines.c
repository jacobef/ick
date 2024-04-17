#include <stdlib.h>
#include "escaped_newlines.h"

#include <string.h>

#include "debug/malloc.h"

struct str_view rm_escaped_newlines(struct str_view input) {
    if (input.n < 2) {
        unsigned char *out_chars = MALLOC(input.n);
        memcpy(out_chars, input.chars, input.n);
        return (struct str_view) { .chars = out_chars, .n = input.n };
    }
    unsigned char *output_chars = MALLOC(input.n);
    const unsigned char *reader = input.chars;
    unsigned char *writer = output_chars;
    // The condition is UB if input.n < 2, but the function should've returned before in that case
    while (reader <= input.chars + input.n - 2) {
        if (reader[0] == '\\' && reader[1] == '\n') {
            reader += 2;
        }
        else {
            *writer = *reader;
            reader++; writer++;
        }
    }
    for (; reader != input.chars + input.n; writer++, reader++) {
        *writer = *reader;
    }
    return (struct str_view){ .chars = output_chars, .n = (size_t)(writer - output_chars) };
}
