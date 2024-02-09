#include "macro_expansion.h"
#include "preprocessor/diagnostics.h"
#include <stdio.h>

static struct earley_rule get_replacement_list_rule(struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &control_line) {
        preprocessor_fatal_error(0, 0, 0, "get_replacement_list_rule called with non-control line");
    }

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_OBJECT_LIKE:
            return *control_line_rule.completed_from.arr[1];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS:
            return *control_line_rule.completed_from.arr[3];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
            return *control_line_rule.completed_from.arr[2];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS:
            return *control_line_rule.completed_from.arr[3];
        default:
            preprocessor_fatal_error(0, 0, 0, "get_replacement_list_rule called with non-define control line");
    }
}

static pp_token_vec get_replacement_tokens(struct earley_rule control_line_rule) {
    struct earley_rule replacement_list_rule = get_replacement_list_rule(control_line_rule);
    struct earley_rule pp_tokens_opt_rule = *replacement_list_rule.completed_from.arr[0];
    pp_token_vec replacement_tokens;
    pp_token_vec_init(&replacement_tokens, 0);
    if (pp_tokens_opt_rule.rhs.tag == OPT_ONE) {
        struct earley_rule pp_tokens_rule = *pp_tokens_opt_rule.completed_from.arr[0];
        for (size_t i = 0; i < pp_tokens_rule.completed_from.n_elements; i++) {
            struct earley_rule pp_token_rule = *pp_tokens_rule.completed_from.arr[i];
            pp_token_vec_append(&replacement_tokens, pp_token_rule.rhs.symbols[0].val.terminal.token);
        }
    }
    return replacement_tokens;
}

static struct preprocessing_token get_macro_name_token(struct earley_rule control_line_rule) {
    return control_line_rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token;
}

void define_object_like_macro(struct earley_rule rule, str_view_macro_args_and_body_map *macros) {
    if (rule.lhs != &control_line || rule.rhs.tag != CONTROL_LINE_DEFINE_OBJECT_LIKE) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }
    struct preprocessing_token macro_name_token = get_macro_name_token(rule);

    if (str_view_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        preprocessor_error(0, 0, 0, "macro already exists");
        return;
    }

    pp_token_vec replacement_tokens = get_replacement_tokens(rule);

    struct macro_args_and_body out = {
            .is_function_like = false,
            .args = NULL,
            .n_args = 0,
            .accepts_varargs = false,
            .replacements = replacement_tokens.arr,
            .n_replacements = replacement_tokens.n_elements
    };

    str_view_macro_args_and_body_map_add(macros, macro_name_token.name, out);
}

static void append_identifier_names(str_view_vec *vec, struct earley_rule identifier_list_rule) {
    for (size_t i = 0; i < identifier_list_rule.completed_from.n_elements; i++) {
        struct earley_rule identifier_rule = *identifier_list_rule.completed_from.arr[i];
        struct preprocessing_token token = identifier_rule.rhs.symbols[0].val.terminal.token;
        str_view_vec_append(vec, token.name);
    }
}

static str_view_vec get_args(struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &control_line || (control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    str_view_vec args;
    str_view_vec_init(&args, 0);

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
            struct earley_rule identifier_list_opt_rule = *control_line_rule.completed_from.arr[2];
            if (identifier_list_opt_rule.rhs.tag == OPT_ONE) {
                struct earley_rule identifier_list_rule = *identifier_list_opt_rule.completed_from.arr[0];
                append_identifier_names(&args, identifier_list_rule);
            }
            break;
        }
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
            break;
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS: {
            struct earley_rule identifier_list_rule = *control_line_rule.completed_from.arr[2];
            for (size_t i = 0; i < identifier_list_rule.completed_from.n_elements; i++) {
                struct earley_rule identifier_rule = *identifier_list_rule.completed_from.arr[i];
                struct preprocessing_token token = identifier_rule.rhs.symbols[0].val.terminal.token;
                str_view_vec_append(&args, token.name);
            }
            break;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    return args;
}

void define_function_like_macro(struct earley_rule rule, str_view_macro_args_and_body_map *macros) {
    if (rule.lhs != &control_line || (rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_function_like_macro is not an function-like macro");
    }
    struct preprocessing_token macro_name_token = rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token;
    if (str_view_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        preprocessor_error(0, 0, 0, "macro already exists");
        return;
    }

    pp_token_vec replacement_tokens = get_replacement_tokens(rule);
    str_view_vec args = get_args(rule);

    struct macro_args_and_body out = {
            .is_function_like = true,
            .args = args.arr,
            .n_args = args.n_elements,
            .accepts_varargs = rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS,
            .replacements = replacement_tokens.arr,
            .n_replacements = replacement_tokens.n_elements
    };

    str_view_macro_args_and_body_map_add(macros, macro_name_token.name, out);
}


void print_macros(str_view_macro_args_and_body_map *macros) {
    for (size_t i = 0; i < macros->n_buckets; i++) {
        NODE_T(str_view, macro_args_and_body) *node = macros->buckets[i];
        while (node != NULL) {
            printf("Macro: ");
            for (size_t j = 0; j < node->key.n; j++) {
                printf("%c", node->key.first[j]);
            }

            if (node->value.is_function_like) {
                printf("(");
                for (size_t arg_index = 0; arg_index < node->value.n_args; arg_index++) {
                    for (size_t k = 0; k < node->value.args[arg_index].n; k++) {
                        printf("%c", node->value.args[arg_index].first[k]);
                    }
                    if (arg_index < node->value.n_args - 1 || node->value.accepts_varargs) {
                        printf(", ");
                    }
                }
                if (node->value.accepts_varargs) {
                    printf("...");
                }
                printf(")");
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


