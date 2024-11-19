#include "sstr.h"

#include "preprocessor/diagnostics.h"

// TODO make sstr.data null-terminated

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

sstr slice(const sstr str, const size_t begin, const size_t end) {
    if (begin > end || begin > str.len || end > str.len) {
        // TODO change to internal_error
        preprocessor_fatal_error(0, 0, 0, "invalid value of begin and/or end in slice");
    }
    return (sstr) { .data = &str.data[begin], .len = end-begin };
}
