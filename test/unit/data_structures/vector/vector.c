#include "data_structures/vector.h"
#include <stdarg.h>

void assert_contains(int_vec *vec, size_t count, ...) {
    va_list args;
    va_start(args, count);
    if (count != vec->n_elements) exit(1);
    for (size_t i = 0; i < vec->n_elements; i++) {
        int expected = va_arg(args, int);
        int actual = vec->arr[i];
        if (expected != actual) exit(1);
    }
}

void test_vector(void) {
    int_vec hmm;
    int_vec_init(&hmm, 0);
    assert_contains(&hmm, 0);

    int_vec_append(&hmm, 5);
    int_vec_append(&hmm, 6);
    int_vec_append(&hmm, 7);
    assert_contains(&hmm, 3,
                    5, 6, 7);
}
