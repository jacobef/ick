#ifndef TEST_SSTR_H
#define TEST_SSTR_H

#include <stddef.h>

// sized string
struct sstr {
    char *chars;
    size_t len;
};

#endif //TEST_SSTR_H
