#include "macro_expansion.h"
#include "preprocessor/diagnostics.h"
#include <stdio.h>

void define_object_like_macro(struct earley_rule rule, str_view_macro_args_body_map *macros) {
    if (rule.lhs != &control_line || rule.rhs.tag != CONTROL_LINE_DEFINE_OBJECT_LIKE) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }
    struct preprocessing_token macro_name_token = rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token;
    if (str_view_macro_args_body_map_contains(macros, macro_name_token.name)) {
        preprocessor_error(0, 0, 0, "macro already exists");
        return;
    }

    pp_token_vec replacement_tokens;
    pp_token_vec_init(&replacement_tokens, 0);
    struct earley_rule pp_tokens_opt_rule = *rule.completed_from.arr[1]->completed_from.arr[0];
    if (pp_tokens_opt_rule.rhs.tag == OPT_ONE) {
        struct earley_rule pp_tokens_rule = *pp_tokens_opt_rule.completed_from.arr[0];
        for (size_t i = 0; i < pp_tokens_rule.completed_from.n_elements; i++) {
            struct earley_rule pp_token_rule = *pp_tokens_rule.completed_from.arr[i];
            pp_token_vec_append(&replacement_tokens, pp_token_rule.rhs.symbols[0].val.terminal.token);
        }
    }

    struct macro_args_body out = {
            .args = (struct str_view) { .first = NULL, .n = 0},
            .n_args = 0,
            .accepts_varargs = false,
            .replacements = replacement_tokens.arr,
            .n_replacements = replacement_tokens.n_elements
    };

    str_view_macro_args_body_map_add(macros, macro_name_token.name, out);
}


void print_macros(str_view_macro_args_body_map *macros) {
    for (size_t i = 0; i < macros->n_buckets; i++) {
        NODE_T(str_view, macro_args_body) *node = macros->buckets[i];
        while (node != NULL) {
            printf("Macro: ");
            for (size_t j = 0; j < node->key.n; j++) {
                printf("%c", node->key.first[j]);
            }
            printf(" -> ");
            for (size_t j = 0; j < node->value.n_replacements; j++) {
                const struct preprocessing_token token = node->value.replacements[j];
                if (token.after_whitespace && j != 0) printf(" ");
                for (size_t k = 0; k < token.name.n; k++) {
                    printf("%c", token.name.first[k]);
                }
            }
            printf("\n");
            node = node->next;
        }
    }
}


