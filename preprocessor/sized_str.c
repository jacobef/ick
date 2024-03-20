#include "sized_str.h"

bool str_view_cstr_eq(const struct str_view view, const unsigned char *cstr) {
    for (size_t i = 0; i < view.n; i++) {
        if (cstr[i] == '\0' || view.first[i] != cstr[i]) return false;
    }
    return cstr[view.n] == '\0';
}
