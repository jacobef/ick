#include "sized_str.h"

bool str_view_cstr_eq(const struct str_view view, const char *const cstr) {
    for (size_t i = 0; i < view.n; i++) {
        if (cstr[i] == '\0' || view.chars[i] != cstr[i]) return false;
    }
    return cstr[view.n] == '\0';
}

bool str_views_eq(const struct str_view t1, const struct str_view t2) {
    if (t1.n != t2.n) return false;
    for (size_t i = 0; i < t1.n; i++) {
        if (t1.chars[i] != t2.chars[i]) return false;
    }
    return true;
}
