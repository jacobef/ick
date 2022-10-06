#ifndef ICK_C_TYPE_TOKENS_H
#define ICK_C_TYPE_TOKENS_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t c_size_t;

struct c_pointer_type_token {
    struct c_type_token *to;
    bool is_restrict;
    bool is_const;
};

struct c_array_type_token {
    c_size_t size;
    struct c_type_token *of;
};

struct c_function_type_token {
    struct c_type_token *returns;
    struct c_type_token **args; // array of pointers
    c_size_t n_args;
};

struct c_basic_type_token {
    bool is_const;
    enum { SIGNED, UNSIGNED, NO_SIGN } sign;
    enum { CHAR, SHORT, INT, FLOAT, DOUBLE } type;
};

struct c_type_token {
    enum { POINTER, ARRAY, FUNCTION, BASIC_TYPE } tag;
    union {
        struct c_pointer_type_token pointer;
        struct c_array_type_token array;
        struct c_function_type_token function;
        struct c_basic_type_token basic_type;
    } val;
};

#endif //ICK_C_TYPE_TOKENS_H
