#ifndef TEST_LINES_H
#define TEST_LINES_H

#include <stddef.h>

struct line {
    const char *chars;
    size_t n_chars;
};
struct lines {
    struct line *lines;
    size_t n_lines;
    char *chars;
    size_t n_chars;
};

struct lines get_lines(char *chars, size_t n_chars);


#endif //TEST_LINES_H
