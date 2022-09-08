#include "map.h"

// TODO make this a real hash function
size_t hash_cstr(char *cstr, size_t capacity) {
    size_t chars_sum = 0;
    for (char *c = cstr; *c != '\0'; c++) {
        chars_sum += (size_t)(*c);
    }
    return chars_sum % capacity;
}

size_t hash_char(char chr, size_t capacity) {
    return (chr * capacity) % capacity;
}

bool chars_equal(char c1, char c2) {
    return c1 == c2;
}

bool cstrs_equal(char *str1, char *str2) {
    return str1 != NULL && str2 != NULL && strcmp(str1, str2) == 0;
}
