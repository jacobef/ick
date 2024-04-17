#include "sized_str.h"

bool str_view_cstr_eq(const struct str_view view, const unsigned char *cstr) {
    for (size_t i = 0; i < view.n; i++) {
        if (cstr[i] == '\0' || view.chars[i] != cstr[i]) return false;
    }
    return cstr[view.n] == '\0';
}

bool str_views_eq(struct str_view t1, struct str_view t2) {
    size_t i1, i2;
    for (i1 = 0, i2 = 0; i1 < t1.n && i2 < t2.n; i1++, i2++) {
        if (t1.chars[i1] != t2.chars[i2]) return false;
    }
    return i1 == t1.n && i2 == t2.n;
}
