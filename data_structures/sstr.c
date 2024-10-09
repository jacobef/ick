#include "sstr.h"

bool sstr_cstr_eq(const sstr view, const char *const cstr) {
    for (size_t i = 0; i < view.len; i++) {
        if (cstr[i] == '\0' || view.data[i] != cstr[i]) return false;
    }
    return cstr[view.len] == '\0';
}

bool sstrs_eq(const sstr t1, const sstr t2) {
    if (t1.len != t2.len) return false;
    for (size_t i = 0; i < t1.len; i++) {
        if (t1.data[i] != t2.data[i]) return false;
    }
    return true;
}
