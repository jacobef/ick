#ifndef TEST_LINES_H
#define TEST_LINES_H

#include <stddef.h>
#include <stdbool.h>

struct str_view {
    const unsigned char *first;
    size_t n;
};

bool str_view_cstr_eq(struct str_view view, const unsigned char *cstr);

#endif //TEST_LINES_H
