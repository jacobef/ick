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

static str_view_vec get_macro_params(struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &control_line || (control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    str_view_vec params;
    str_view_vec_init(&params, 0);

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
            struct earley_rule identifier_list_opt_rule = *control_line_rule.completed_from.arr[2];
            if (identifier_list_opt_rule.rhs.tag == OPT_ONE) {
                struct earley_rule identifier_list_rule = *identifier_list_opt_rule.completed_from.arr[0];
                append_identifier_names(&params, identifier_list_rule);
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
                str_view_vec_append(&params, token.name);
            }
            break;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    return params;
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
    str_view_vec args = get_macro_params(rule);

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

struct macro_use_info get_macro_use_info(const pp_token_vec tokens, size_t macro_inv_start, macro_args_and_body macro_def) {
    if (macro_inv_start == tokens.n_elements - 1 || !token_is_str(tokens.arr[macro_inv_start + 1], "(")) {
        // object-like use
        if (macro_def.is_function_like) {
            // object-like use of function-like macro
            preprocessor_fatal_error(0, 0, 0, "attempted to use a function-like macro without an argument list (i.e. without parenthesis)");
        }
        // object-like use of object-like macro
        return (struct macro_use_info) {
            .macro_name = tokens.arr[macro_inv_start].name,
            .end_index = macro_inv_start + 1,
            .args = NULL,
            .n_args = 0,
            .is_function_like = false
        };
    } else if (!macro_def.is_function_like) {
        // function-like use of object-like macro
        preprocessor_fatal_error(0, 0, 0, "attempted to use object-like macro as a function-like macro, i.e. with parenthesis");
    }
    // function-like use of function-like macro

    given_macro_arg_vec given_args;
    given_macro_arg_vec_init(&given_args, 0);
    pp_token_vec current_arg;
    pp_token_vec_init(&current_arg, 0);
    int net_open_parens = 1;
    size_t i = macro_inv_start + 2; // skip the macro name and first open paren

    for (; i < tokens.n_elements; i++) {
        if (token_is_str(tokens.arr[i], "(")) {
            net_open_parens++;
        } else if (token_is_str(tokens.arr[i], ")")) {
            net_open_parens--;
            if (net_open_parens == 0) break;
        }
        if (net_open_parens == 1 && token_is_str(tokens.arr[i], ",")) {
            // argument-separating comma
            given_macro_arg_vec_append(&given_args, (struct given_macro_arg) {
                .tokens = current_arg.arr,
                .n_tokens = current_arg.n_elements
            });
            pp_token_vec empty;
            pp_token_vec_init(&empty, 0);
            current_arg = empty;
        } else {
            pp_token_vec_append(&current_arg, tokens.arr[i]);
        }
    }
    if (net_open_parens > 0) {
        preprocessor_fatal_error(0, 0, 0, "%d open parens in macro call are unmatched by a closed paren", net_open_parens);
    }

    bool macro_can_take_one_arg = (macro_def.n_args == 1 && !macro_def.accepts_varargs) || (macro_def.n_args == 0 && macro_def.accepts_varargs);
    if (current_arg.n_elements > 0
        || (i >= 2 && token_is_str(tokens.arr[i-2], ","))
        || (current_arg.n_elements == 0 && given_args.n_elements == 0 && macro_can_take_one_arg)) {
        // current_arg.n_elements > 0: non-empty arg at end, e.g. A(x, y)
        // i >= 2 && token_is_str(tokens.arr[i-2], ",")): empty arg at end, e.g. A(x,)
        // current_arg.n_elements == 0 && given_args.n_elements == 0 && macro_can_take_one_arg: handles empty parens (e.g. X()) for definitions like `#define X()` and `#define X(...)`
        given_macro_arg_vec_append(&given_args, (struct given_macro_arg) {
                .tokens = current_arg.arr,
                .n_tokens = current_arg.n_elements
        });
    }

    if (!macro_def.accepts_varargs && given_args.n_elements != macro_def.n_args) {
        preprocessor_fatal_error(0, 0, 0,
            "wrong number of args given to macro; expected %zu, got %zu", macro_def.n_args, given_args.n_elements);
    } else if (macro_def.accepts_varargs && given_args.n_elements < macro_def.n_args + 1) {
        preprocessor_fatal_error(0, 0, 0,
            "too few args given to varargs macro; expected at least %zu, got %zu", macro_def.n_args + 1, given_args.n_elements);
    }

    return (struct macro_use_info) {
        .macro_name = tokens.arr[macro_inv_start].name,
        .end_index = i,
        .args = given_args.arr,
        .n_args = given_args.n_elements,
        .is_function_like = true
    };
}

void reconstruct_macro_use(struct macro_use_info info) {
    // Print the macro name
    printf("Macro use: %.*s", (int)info.macro_name.n, (const char*)info.macro_name.first);

    // If the macro is function-like and there are arguments, print them within parentheses
    if (info.is_function_like) {
        printf("(");
        for (size_t i = 0; i < info.n_args; i++) {
            // Print each argument
            for (size_t j = 0; j < info.args[i].n_tokens; j++) {
                if (info.args[i].tokens[j].after_whitespace && j != 0) {
                    printf(" ");
                }
                printf("%.*s", (int)info.args[i].tokens[j].name.n, (const char*)info.args[i].tokens[j].name.first);
            }
            if (i < info.n_args - 1) {
                printf(", ");
            }
        }
        printf(")");
        printf(" - Number of arguments given: %zu", info.n_args);
    }
    printf("\n");
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


