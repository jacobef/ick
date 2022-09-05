//
// Created by Jacob Friedman on 8/24/22.
//

#ifndef TEST_VECTOR_H
#define TEST_VECTOR_H

#include "data_structures/vector.h"
#include <stddef.h>

void assert_contains(int_vec *vec, size_t count, ...);
void test_vector(void);
#endif //TEST_VECTOR_H
