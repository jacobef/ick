//
// Created by Jacob Friedman on 8/29/22.
//

#include "data_structures/vector.h"
#include "lines.h"

struct lines get_lines(const char *chars, size_t n_chars) {
    size_t n_lines = 1;
    for (size_t i = 0; i+1 < n_chars; i++) {
        if (chars[i] == '\n') n_lines++;
    }
    struct lines result;
    result.n_lines = n_lines;
    result.lines = malloc(sizeof(struct line) * n_lines);
    result.chars = chars;
    result.n_chars = n_chars;

    size_t chr_i = 0;
    size_t ln_i = 0;
    while (ln_i < n_lines) {
        result.lines[ln_i].chars = &chars[chr_i];
        size_t first_char_i = chr_i;
        while (chr_i < n_chars - 1 && chars[chr_i] != '\n') {
            chr_i++;
        }
        result.lines[ln_i].n_chars = chr_i - first_char_i + 1;
        ln_i++;
        chr_i++;
    }
    return result;
}
