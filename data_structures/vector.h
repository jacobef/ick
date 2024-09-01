#ifndef ICK_DATA_STRUCTURES_VECTOR_VECTOR_H
#define ICK_DATA_STRUCTURES_VECTOR_VECTOR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug/reminder.h"
#include "debug/malloc.h"

#define DEFINE_VEC_TYPE(_type) \
typedef struct _type##_vec {   \
    _type *arr;                \
    size_t n_elements;         \
    size_t capacity;           \
} _type##_vec;

#define DEFINE_VEC_NEW_FUNCTION(_type)                \
__attribute__((unused))                               \
static _type##_vec _type##_vec_new(size_t capacity) { \
    REMEMBER("free " #_type " vector internals");     \
    if (capacity == 0) capacity = 1;                  \
    return (struct _type##_vec) {                     \
        .arr = MALLOC(capacity*sizeof(_type)),        \
        .n_elements = 0,                              \
        .capacity = capacity                          \
    };                                                \
}

#define DEFINE_VEC_APPEND_FUNCTION(_type)                                       \
__attribute__((unused))                                                         \
static void _type##_vec_append(_type##_vec *const vec_p, const _type element) { \
    if (vec_p->n_elements == vec_p->capacity) {                                 \
        vec_p->arr = REALLOC(vec_p->arr, vec_p->capacity*2 * sizeof(_type));    \
        vec_p->capacity *= 2;                                                   \
    }                                                                           \
    vec_p->arr[vec_p->n_elements] = element;                                    \
    vec_p->n_elements++;                                                        \
}

// TODO make efficient
#define DEFINE_VEC_APPEND_ALL_FUNCTION(_type)                                        \
__attribute__((unused))                                                              \
static void _type##_vec_append_all(_type##_vec *const dest, const _type##_vec src) { \
    for (size_t i = 0; i < src.n_elements; i++) {                                    \
        _type##_vec_append(dest, src.arr[i]);                                        \
    }                                                                                \
}

// TODO make efficient
#define DEFINE_VEC_APPEND_ALL_ARR_FUNCTION(_type)                                                         \
__attribute__((unused))                                                                                   \
static void _type##_vec_append_all_arr(_type##_vec *const dest, const _type *const arr, const size_t n) { \
    for (size_t i = 0; i < n; i++) {                                                                      \
        _type##_vec_append(dest, arr[i]);                                                                 \
    }                                                                                                     \
}

#define DEFINE_VEC_COPY_FUNCTION(_type)                      \
__attribute__((unused))                                      \
static _type##_vec _type##_vec_copy(const _type##_vec vec) { \
    _type##_vec copy = _type##_vec_new(vec.capacity);        \
    _type##_vec_append_all(&copy, vec);                      \
    return copy;                                             \
}

#define DEFINE_VEC_COPY_ARR_FUNCTION(_type)                                            \
__attribute__((unused))                                                                \
static _type##_vec _type##_vec_copy_from_arr(const _type *const arr, const size_t n) { \
    _type##_vec copy = _type##_vec_new(n);                                             \
    _type##_vec_append_all_arr(&copy, arr, n);                                         \
    return copy;                                                                       \
}

#define DEFINE_VEC_FREE_INTERNALS_FUNCTION(_type)                        \
__attribute__((unused))                                                  \
static void _type##_vec_free_internals(const _type##_vec *const vec_p) { \
    FREE(vec_p->arr);                                                    \
    REMEMBERED_TO("free " #_type " vector internals");                   \
}

#define DEFINE_VEC_TYPE_AND_FUNCTIONS(_type)          \
    DEFINE_VEC_TYPE(_type)                            \
    DEFINE_VEC_NEW_FUNCTION(_type)                    \
    DEFINE_VEC_APPEND_FUNCTION(_type)                 \
    DEFINE_VEC_APPEND_ALL_FUNCTION(_type)             \
    DEFINE_VEC_APPEND_ALL_ARR_FUNCTION(_type)         \
    DEFINE_VEC_COPY_FUNCTION(_type)                   \
    DEFINE_VEC_COPY_ARR_FUNCTION(_type)               \
    DEFINE_VEC_FREE_INTERNALS_FUNCTION(_type)         \

DEFINE_VEC_TYPE_AND_FUNCTIONS(int)

#endif // ICK_DATA_STRUCTURES_VECTOR_VECTOR_H
