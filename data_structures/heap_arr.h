#ifndef ICK_DATA_STRUCTURES_HEAP_ARR_H
#define ICK_DATA_STRUCTURES_HEAP_ARR_H

#include <stddef.h>
#include <string.h>
#include "debug/malloc.h"

#define DEFINE_HARR_TYPE(_type) typedef struct _type##_harr { \
    _type *data; size_t len;                                  \
} _type##_harr;                                               \

#define DEFINE_HARR_COPY_FUNCTION(_type)                        \
__attribute__((unused))                                         \
static _type##_harr _type##_harr_copy(_type##_harr arr) {       \
    _type *out_data = MALLOC(sizeof(arr.data[0]) * arr.len);    \
    memcpy(out_data, arr.data, sizeof(arr.data[0]) * arr.len);  \
    return (_type##_harr) { .data = out_data, .len = arr.len }; \
}

#define DEFINE_HARR_TYPE_AND_FUNCTIONS(_type) \
    DEFINE_HARR_TYPE(_type)                   \
    DEFINE_HARR_COPY_FUNCTION(_type)

#endif //ICK_DATA_STRUCTURES_HEAP_ARR_H
