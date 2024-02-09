#ifndef MACROS_H
#define MACROS_H

#include <stdbool.h>
#include "preprocessor/pp_token.h"
#include "preprocessor/parser.h"
#include "data_structures/map.h"


struct macro_args_and_body {
    struct str_view *args;
    size_t n_args;
    bool accepts_varargs;
    bool is_function_like;
    struct preprocessing_token *replacements;
    size_t n_replacements;
};

static bool sstr_views_equal(struct str_view t1, struct str_view t2) {
    size_t i1, i2;
    for (i1 = 0, i2 = 0; i1 < t1.n && i2 < t2.n; i1++, i2++) {
        if (t1.first[i1] != t2.first[i2]) return false;
    }
    return i1 == t1.n && i2 == t2.n;
}

static size_t hash_sstr_view(struct str_view token_name, size_t capacity) {
    size_t chars_sum = 0;
    for (size_t i = 0; i < token_name.n; i++) {
        chars_sum += (size_t)(token_name.first[i]);
    }
    return chars_sum % capacity;
}

typedef struct str_view str_view;
DEFINE_VEC_TYPE_AND_FUNCTIONS(str_view)
typedef struct macro_args_and_body macro_args_and_body;
DEFINE_MAP_TYPE_AND_FUNCTIONS(str_view, macro_args_and_body, hash_sstr_view, sstr_views_equal)

void define_object_like_macro(struct earley_rule rule, str_view_macro_args_and_body_map *macros);
void define_function_like_macro(struct earley_rule rule, str_view_macro_args_and_body_map *macros);
void print_macros(str_view_macro_args_and_body_map *macros);

#endif //MACROS_H
