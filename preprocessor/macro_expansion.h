#ifndef MACROS_H
#define MACROS_H

#include <stdbool.h>
#include "preprocessor/pp_token.h"
#include "preprocessor/parser.h"
#include "data_structures/map.h"
#include "data_structures/heap_arr.h"
#include "data_structures/sstr.h"

DEFINE_VEC_TYPE_AND_FUNCTIONS(sstr)

typedef struct macro_args_and_body {
    sstr_harr args;
    bool accepts_varargs;
    bool is_function_like;
    pp_token_harr replacements;
} macro_args_and_body;

static size_t hash_ssstr(const sstr token_name, const size_t capacity) {
    size_t chars_sum = 0;
    for (size_t i = 0; i < token_name.len; i++) {
        chars_sum += (size_t)(token_name.data[i]);
    }
    return chars_sum % capacity;
}

DEFINE_MAP_TYPE_AND_FUNCTIONS(sstr, macro_args_and_body, hash_ssstr, sstrs_eq)

typedef struct token_with_ignore_list {
    struct preprocessing_token token;
    sstr_vec dont_replace;
} token_with_ignore_list;
DEFINE_VEC_TYPE_AND_FUNCTIONS(token_with_ignore_list)
DEFINE_VEC_TYPE_AND_FUNCTIONS(token_with_ignore_list_harr)

struct macro_use_info {
    sstr macro_name;
    bool after_whitespace;
    size_t end_index;
    token_with_ignore_list_harr_harr args; // empty if object-like, but don't depend on that behavior
    token_with_ignore_list_harr vararg_tokens; // empty if doesn't accept varargs, but don't depend on that behavior
    bool is_function_like;
    sstr_vec dont_replace;
    bool is_valid;
};

void define_object_like_macro(struct earley_rule rule, sstr_macro_args_and_body_map *macros);
void define_function_like_macro(struct earley_rule rule, sstr_macro_args_and_body_map *macros);
void print_macros(const sstr_macro_args_and_body_map *macros);
void reconstruct_macro_use(struct macro_use_info info);
pp_token_harr replace_macros(pp_token_harr tokens, sstr_macro_args_and_body_map macro_map, enum exclude_from_detection exclude_concatenation_type);

#endif //MACROS_H
