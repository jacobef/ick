#ifndef ICK_DATA_STRUCTURES_VECTOR_VECTOR_H
#define ICK_DATA_STRUCTURES_VECTOR_VECTOR_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFINE_VEC_TYPE(_type)          \
typedef struct _type##_vec {          \
    _type *arr;                       \
    size_t n_elements;              \
    size_t capacity;                  \
} _type##_vec;

#define DEFINE_VEC_INIT_FUNCTION(_type)                                      \
static inline void _type##_vec##_init(_type##_vec *vec_p, size_t capacity) { \
    if (capacity == 0) capacity = 1;                                         \
    vec_p->arr = malloc(capacity*sizeof(_type));                             \
    vec_p->n_elements = 0;                                                 \
    vec_p->capacity = capacity;                                              \
}

#define DEFINE_VEC_AT_FUNCTION(_type)                                      \
static inline _type _type##_vec##_at(const _type##_vec *vec_p, size_t i) { \
    return vec_p->arr[i];                                                  \
}


#define DEFINE_VEC_EXPAND_FUNCTION(_type)                                            \
static inline void _type##_vec##_expand(_type##_vec *vec_p, size_t extra_capacity) { \
    vec_p->capacity += extra_capacity;                                               \
    vec_p->arr = realloc(vec_p->arr, vec_p->capacity * sizeof(_type));             \
}

#define DEFINE_VEC_SHRINK_RETAIN_CAPACITY_FUNCTION(_type)                                              \
static inline void _type##_vec##_shrink_retain_capacity(_type##_vec *vec_p, size_t shrink_by) { \
    vec_p->n_elements -= shrink_by;                                                           \
}

#define DEFINE_VEC_APPEND_FUNCTION(_type)                                     \
static inline void _type##_vec##_append(_type##_vec *vec_p, _type element) {  \
    if (vec_p->n_elements == vec_p->capacity ) {                            \
        _type##_vec##_expand(vec_p, vec_p->capacity); /* doubles capacity */  \
    }                                                                         \
    vec_p->arr[vec_p->n_elements] = element;                               \
    vec_p->n_elements++;                                                    \
}

#define DEFINE_VEC_COPY_FUNCTION(_type)                                            \
static inline void _type##_vec##_copy(_type##_vec *dest, const _type##_vec *src) { \
    _type##_vec##_shrink_retain_capacity(dest, dest->n_elements);                \
    for (size_t i = 0; i < src->n_elements; i++) {                               \
        _type##_vec##_append(dest, _type##_vec##_at(src, i));                      \
    }                                                                              \
}

#define DEFINE_VEC_POP_FUNCTION(_type)                     \
static inline void _type##_vec##_pop(_type##_vec *vec_p) { \
    if (vec_p->n_elements > 0) vec_p->n_elements--;    \
}

#define DEFINE_VEC_CONTAINS_FUNCTION(_type)                                    \
static inline bool _type##_vec##_contains(_type##_vec *vec_p, _type element) { \
    for (size_t i = 0; i < vec_p->n_elements; i++) {                         \
        if (vec_p->arr[i] == element) return true;                             \
    }                                                                          \
    return false;                                                              \
}

#define DEFINE_VEC_FREE_INTERNALS_FUNCTION(_type)                     \
static inline void _type##_vec##_free_internals(_type##_vec *vec_p) { \
    free(vec_p->arr);                                                 \
}

#define DEFINE_VEC_TYPE_AND_FUNCTIONS(_type)          \
    DEFINE_VEC_TYPE(_type)                            \
    DEFINE_VEC_INIT_FUNCTION(_type)                   \
    DEFINE_VEC_AT_FUNCTION(_type)                     \
    DEFINE_VEC_EXPAND_FUNCTION(_type)                 \
    DEFINE_VEC_SHRINK_RETAIN_CAPACITY_FUNCTION(_type) \
    DEFINE_VEC_APPEND_FUNCTION(_type)                 \
    DEFINE_VEC_COPY_FUNCTION(_type)                   \
    DEFINE_VEC_POP_FUNCTION(_type)                    \
    DEFINE_VEC_FREE_INTERNALS_FUNCTION(_type)

DEFINE_VEC_TYPE_AND_FUNCTIONS(char)
DEFINE_VEC_TYPE_AND_FUNCTIONS(int)

// To avoid namespace pollution
#undef DEFINE_VEC_TYPE
#undef DEFINE_VEC_INIT_FUNCTION
#undef DEFINE_VEC_AT_FUNCTION
#undef DEFINE_VEC_APPEND_FUNCTION
#undef DEFINE_VEC_SHRINK_RETAIN_CAPACITY_FUNCTION
#undef DEFINE_VEC_COPY_FUNCTION
#undef DEFINE_VEC_POP_FUNCTION
#undef DEFINE_VEC_CONTAINS_FUNCTION
#undef DEFINE_VEC_FREE_INTERNALS_FUNCTION

#endif // ICK_DATA_STRUCTURES_VECTOR_VECTOR_H
