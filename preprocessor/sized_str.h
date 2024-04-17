#ifndef TEST_LINES_H
#define TEST_LINES_H

#include <stddef.h>
#include <stdbool.h>

struct str_view {
    const unsigned char *chars;
    size_t n;
};

bool str_view_cstr_eq(struct str_view view, const unsigned char *cstr);
bool str_views_eq(struct str_view t1, struct str_view t2);

#endif //TEST_LINES_H
