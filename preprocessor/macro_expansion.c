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

static struct macro_use_info get_macro_use_info(const token_with_ignore_list_vec tokens, size_t macro_inv_start, macro_args_and_body macro_def) {
    str_view_vec new_dont_replace;
    str_view_vec_init(&new_dont_replace, 0);
    str_view_vec_append_all(&new_dont_replace, &tokens.arr[macro_inv_start].dont_replace);

    if (macro_inv_start == tokens.n_elements - 1 || !token_is_str(tokens.arr[macro_inv_start + 1].token, (const unsigned char*)"(")) {
        // object-like use
        if (macro_def.is_function_like) {
            // object-like use of function-like macro
            return (struct macro_use_info) {
                .is_valid = false
            };
        }
        // object-like use of object-like macro
        return (struct macro_use_info) {
            .macro_name = tokens.arr[macro_inv_start].token.name,
            .end_index = macro_inv_start + 1,
            .args = NULL,
            .n_args = 0,
            .is_function_like = false,
            .dont_replace = new_dont_replace,
            .is_valid = true
        };
    } else if (!macro_def.is_function_like) {
        // function-like use of object-like macro
        return (struct macro_use_info) {
                .macro_name = tokens.arr[macro_inv_start].token.name,
                .end_index = macro_inv_start + 1,
                .args = NULL,
                .n_args = 0,
                .is_function_like = false,
                .dont_replace = new_dont_replace,
                .is_valid = true
        };
    }
    // function-like use of function-like macro

    given_macro_arg_vec given_args;
    given_macro_arg_vec_init(&given_args, 0);
    token_with_ignore_list_vec current_arg;
    token_with_ignore_list_vec_init(&current_arg, 0);
    token_with_ignore_list_vec varargs;
    token_with_ignore_list_vec_init(&varargs, 0);
    bool in_varargs = macro_def.accepts_varargs && macro_def.n_args == 0;
    int net_open_parens = 1;
    size_t i = macro_inv_start + 2; // skip the macro name and first open paren
    for (; i < tokens.n_elements; i++) {
        if (token_is_str(tokens.arr[i].token, (const unsigned char*)"(")) {
            net_open_parens++;
        } else if (token_is_str(tokens.arr[i].token, (const unsigned char*)")")) {
            net_open_parens--;
            if (net_open_parens == 0) {
                i++;
                break;
            }
        } else if (in_varargs) {
            token_with_ignore_list_vec_append(&varargs, tokens.arr[i]);
        }
        if (net_open_parens == 1 && token_is_str(tokens.arr[i].token, (const unsigned char*)",")) {
            // argument-separating comma
            given_macro_arg_vec_append(&given_args, (struct given_macro_arg) {
                .tokens = current_arg.arr,
                .n_tokens = current_arg.n_elements
            });
            if (macro_def.accepts_varargs && given_args.n_elements == macro_def.n_args) {
                in_varargs = true;
            }
            token_with_ignore_list_vec empty;
            token_with_ignore_list_vec_init(&empty, 0);
            current_arg = empty;
        } else {
            token_with_ignore_list_vec_append(&current_arg, tokens.arr[i]);
        }
    }
    if (net_open_parens > 0) {
        preprocessor_fatal_error(0, 0, 0, "%d open parens in macro call are unmatched by a closed paren", net_open_parens);
    }

    bool macro_can_take_one_arg = (macro_def.n_args == 1 && !macro_def.accepts_varargs) || (macro_def.n_args == 0 && macro_def.accepts_varargs);
    if (current_arg.n_elements > 0
        || (i >= 2 && token_is_str(tokens.arr[i-2].token, (const unsigned char*)","))
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
        .macro_name = tokens.arr[macro_inv_start].token.name,
        .end_index = i,
        .args = given_args.arr,
        .n_args = given_args.n_elements,
        .vararg_tokens = varargs.arr,
        .n_vararg_tokens = varargs.n_elements,
        .is_function_like = true,
        .dont_replace = new_dont_replace,
        .is_valid = true
    };
}

typedef unsigned char uchar;
DEFINE_VEC_TYPE_AND_FUNCTIONS(uchar)

static struct str_view stringify(struct given_macro_arg arg) {
    uchar_vec out;
    uchar_vec_init(&out, 2);
    uchar_vec_append(&out, '"');
    for (size_t i = 0; i < arg.n_tokens; i++) {
        if (arg.tokens[i].token.after_whitespace && i != 0) {
            uchar_vec_append(&out, ' ');
        }
        for (size_t j = 0; j < arg.tokens[i].token.name.n; j++) {
            if ((arg.tokens[i].token.type == STRING_LITERAL || arg.tokens[i].token.type == CHARACTER_CONSTANT)
            && (arg.tokens[i].token.name.first[j] == '\\' || arg.tokens[i].token.name.first[j] == '"')) {
                uchar_vec_append(&out, '\\');
            }
            uchar_vec_append(&out, arg.tokens[i].token.name.first[j]);
        }
    }
    uchar_vec_append(&out, '"');
    return (struct str_view) { .first = out.arr, .n = out.n_elements };
}

static struct str_view concatenate(struct str_view arg1, struct str_view arg2) {
    uchar_vec out;
    uchar_vec_init(&out, arg1.n + arg2.n);
    uchar_vec_append_all_arr(&out, arg1.first, arg1.n);
    uchar_vec_append_all_arr(&out, arg2.first, arg2.n);
    return (struct str_view) { .first = out.arr, .n = out.n_elements };
}

static ssize_t get_arg_index(struct str_view token_name, struct macro_args_and_body macro_def) {
    for (size_t i = 0; i < macro_def.n_args; i++) {
        if (sstr_views_equal(token_name, macro_def.args[i])) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static bool str_view_vec_contains(str_view_vec vec, struct str_view view) {
    for (size_t i = 0; i < vec.n_elements; i++) {
        if (sstr_views_equal(vec.arr[i], view)) {
            return true;
        }
    }
    return false;
}

static str_view_vec copy_str_view_vec(str_view_vec original) {
    str_view_vec out;
    str_view_vec_init(&out, original.n_elements);
    str_view_vec_append_all(&out, &original);
    return out;
}

static token_with_ignore_list_vec replace_macros_helper(token_with_ignore_list_vec tokens, size_t scan_start, str_view_macro_args_and_body_map macro_map);

static token_with_ignore_list_vec replace_arg(struct given_macro_arg arg, str_view_macro_args_and_body_map macro_map, struct str_view macro_name, str_view_vec ignore_list) {
    token_with_ignore_list_vec arg_tokens;
    token_with_ignore_list_vec_init(&arg_tokens, 0);
    for (size_t i = 0; i < arg.n_tokens; i++) {
        str_view_vec_append_all(&ignore_list, &arg.tokens[i].dont_replace);
        token_with_ignore_list_vec_append(&arg_tokens, (struct token_with_ignore_list) {
                .token = arg.tokens[i].token, .dont_replace = copy_str_view_vec(ignore_list)
        });
    }
    token_with_ignore_list_vec out = replace_macros_helper(arg_tokens, 0, macro_map);
    for (size_t i = 0; i < out.n_elements; i++) {
        str_view_vec_append(&out.arr[i].dont_replace, macro_name);
    }
    return out;
}

static pp_token_vec eval_stringifies(struct macro_args_and_body macro_info, struct macro_use_info use_info) {
    pp_token_vec out;
    pp_token_vec_init(&out, 0);
    for (size_t i = 0; i < macro_info.n_replacements;) {
        if (i != macro_info.n_replacements - 1 && token_is_str(macro_info.replacements[i], "#") && macro_info.is_function_like) {
            ssize_t arg_index = get_arg_index(macro_info.replacements[i+1].name, macro_info);
            if (arg_index == -1) {
                preprocessor_fatal_error(0, 0, 0, "can't stringify non-argument");
            }

            pp_token_vec_append(&out, (struct preprocessing_token){
                            .name = stringify(use_info.args[arg_index]),
                            .type=STRING_LITERAL,
                            .after_whitespace = macro_info.replacements[i].after_whitespace
                    });
            i += 2;
        } else if (i == macro_info.n_replacements - 1 && token_is_str(macro_info.replacements[i], "#") && macro_info.is_function_like) {
            preprocessor_fatal_error(0, 0, 0, "# operator can't appear at the end of a macro");
        }
        else {
            pp_token_vec_append(&out, macro_info.replacements[i]);
            i++;
        }
    }
    return out;
}

typedef _Bool boolean;
DEFINE_VEC_TYPE_AND_FUNCTIONS(boolean)

static token_with_ignore_list_vec get_replacement(struct macro_args_and_body macro_info, struct macro_use_info use_info, str_view_macro_args_and_body_map macro_map) {
    // add varargs to use info
    given_macro_arg_vec new_given_args;
    given_macro_arg_vec_init(&new_given_args, use_info.n_args + 1);
    given_macro_arg_vec_append_all_arr(&new_given_args, use_info.args, macro_info.n_args); // not a typo
    given_macro_arg_vec_append(&new_given_args, (struct given_macro_arg) { .tokens = use_info.vararg_tokens, .n_tokens = use_info.n_vararg_tokens });
    use_info.args = new_given_args.arr;
    use_info.n_args = new_given_args.n_elements;

    // add varargs to macro info
    str_view_vec new_arg_names;
    str_view_vec_init(&new_arg_names, macro_info.n_args + 1);
    str_view_vec_append_all_arr(&new_arg_names, macro_info.args, macro_info.n_args);
    str_view_vec_append(&new_arg_names, (struct str_view) { .first = "__VA_ARGS__", .n = sizeof("__VA_ARGS__") - 1 });
    macro_info.args = new_arg_names.arr;
    macro_info.n_args = new_arg_names.n_elements;

    token_with_ignore_list_vec replaced_tokens;
    token_with_ignore_list_vec_init(&replaced_tokens, 0);
    boolean_vec needs_concat;
    boolean_vec_init(&needs_concat, 0);

    str_view_vec dont_replace_list = copy_str_view_vec(use_info.dont_replace);
    str_view_vec_append(&dont_replace_list, use_info.macro_name);

    bool ignore_stringify_left = false;
    pp_token_vec stringifies_expanded = eval_stringifies(macro_info, use_info);
    for (size_t i = 0; i < stringifies_expanded.n_elements;) {
        if (i != stringifies_expanded.n_elements - 1 && token_is_str(stringifies_expanded.arr[i+1], "##")) {
            if (i+1 == stringifies_expanded.n_elements - 1) {
                preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
            }

            ssize_t arg1_index = get_arg_index(stringifies_expanded.arr[i].name, macro_info);
            ssize_t arg2_index = get_arg_index(stringifies_expanded.arr[i+2].name, macro_info);

            if (!ignore_stringify_left) {
                if (arg1_index == -1) {
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = stringifies_expanded.arr[i], .dont_replace = copy_str_view_vec(dont_replace_list)
                    });
                    boolean_vec_append(&needs_concat, false);
                } else if (use_info.args[arg1_index].n_tokens == 0) {
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = {
                                    .name = {.first = "", .n = 0},
                                    .after_whitespace = stringifies_expanded.arr[i].after_whitespace,
                                    // type intentionally omitted
                            },
                            .dont_replace = copy_str_view_vec(dont_replace_list)
                    });
                    boolean_vec_append(&needs_concat, false);
                } else {
                    token_with_ignore_list_vec_append_all_arr(&replaced_tokens, use_info.args[arg1_index].tokens,
                                                              use_info.args[arg1_index].n_tokens);
                    for (size_t j = 0; j < use_info.args[arg1_index].n_tokens; j++) {
                        boolean_vec_append(&needs_concat, false);
                    }
                }
            }

            if (arg2_index == -1) {
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = stringifies_expanded.arr[i+2], .dont_replace = copy_str_view_vec(dont_replace_list)
                });
                boolean_vec_append(&needs_concat, true);
            } else if (use_info.args[arg2_index].n_tokens == 0) {
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = {
                                .name = { .first = "", .n = 0 },
                                .after_whitespace = stringifies_expanded.arr[i].after_whitespace,
                                // type intentionally omitted
                        },
                        .dont_replace = copy_str_view_vec(dont_replace_list)
                });
                boolean_vec_append(&needs_concat, true);
            } else {
                token_with_ignore_list_vec_append_all_arr(&replaced_tokens, use_info.args[arg2_index].tokens, use_info.args[arg2_index].n_tokens);
                boolean_vec_append(&needs_concat, true);
                for (size_t j = 1; j < use_info.args[arg2_index].n_tokens; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            ignore_stringify_left = true;
            i += 2;
        } else if (i == 0 && token_is_str(stringifies_expanded.arr[i], "##")) {
            preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
        } else if (ignore_stringify_left) {
            i++;
            ignore_stringify_left = false;
        } else {
            ssize_t arg_index = get_arg_index(stringifies_expanded.arr[i].name, macro_info);
            if (arg_index == -1) {
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                    .token = stringifies_expanded.arr[i], .dont_replace = copy_str_view_vec(dont_replace_list) }
                );
                boolean_vec_append(&needs_concat, false);
            } else {
                token_with_ignore_list_vec new_arg = replace_arg(use_info.args[arg_index], macro_map, use_info.macro_name, use_info.dont_replace);
                token_with_ignore_list_vec_append_all(&replaced_tokens, &new_arg);
                for (size_t j = 0; j < new_arg.n_elements; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            ignore_stringify_left = false;
            i++;
        }
    }

    token_with_ignore_list_vec out;
    token_with_ignore_list_vec_init(&out, 0);
    for (size_t i = 0; i < replaced_tokens.n_elements; i++) {
        if (needs_concat.arr[i]) {
            struct str_view concat_result = concatenate(out.arr[out.n_elements - 1].token.name, replaced_tokens.arr[i].token.name);
            bool token_valid = is_valid_token(concat_result, EXCLUDE_HEADER_NAME);
            if (!token_valid && concat_result.n != 0) {
                preprocessor_fatal_error(0, 0, 0, "concat result %.*s is not a valid token", (int)concat_result.n, concat_result.first);
            }
            out.arr[out.n_elements - 1].token.name = concat_result;
            if (token_valid) {
                out.arr[out.n_elements - 1].token.type = get_token_type_from_str(concat_result, EXCLUDE_HEADER_NAME);
            }
        } else {
            token_with_ignore_list_vec_append(&out, replaced_tokens.arr[i]);
        }
    }

    return out;
}

static token_with_ignore_list_vec replace_macros_helper(token_with_ignore_list_vec tokens, size_t scan_start, str_view_macro_args_and_body_map macro_map) {
    token_with_ignore_list_vec out;
    token_with_ignore_list_vec_init(&out, 0);

    bool ignore_replacements = false;
    size_t new_scan_start = tokens.n_elements;
    for (size_t i = 0; i < tokens.n_elements;) {
        if (str_view_macro_args_and_body_map_contains(&macro_map, tokens.arr[i].token.name)
        && !ignore_replacements
        && !str_view_vec_contains(tokens.arr[i].dont_replace, tokens.arr[i].token.name)
        && i >= scan_start) {
            struct macro_args_and_body macro_info = str_view_macro_args_and_body_map_get(&macro_map, tokens.arr[i].token.name);
            struct macro_use_info use_info = get_macro_use_info(tokens, i, macro_info);
            if (!use_info.is_valid) {
                token_with_ignore_list_vec_append(&out, tokens.arr[i]);
                i++;
                continue;
            }
            token_with_ignore_list_vec replaced_tokens = get_replacement(macro_info, use_info, macro_map);
            token_with_ignore_list_vec_append_all(&out, &replaced_tokens);
            new_scan_start = i;
            i = use_info.end_index;
            ignore_replacements = true;
        } else {
            token_with_ignore_list_vec_append(&out, tokens.arr[i]);
            i++;
        }
    }
    if (new_scan_start == tokens.n_elements) {
        return out;
    } else {
        return replace_macros_helper(out, new_scan_start, macro_map);
    }
}

pp_token_vec replace_macros(pp_token_vec tokens, str_view_macro_args_and_body_map macro_map) {
    token_with_ignore_list_vec tokens_with_ignore_list;
    token_with_ignore_list_vec_init(&tokens_with_ignore_list, tokens.n_elements);
    for (size_t i = 0; i < tokens.n_elements; i++) {
        if (!token_is_str(tokens.arr[i], "\n")) {
            str_view_vec empty;
            str_view_vec_init(&empty, 0);
            token_with_ignore_list_vec_append(&tokens_with_ignore_list, (struct token_with_ignore_list) {
                    .token = tokens.arr[i],
                    .dont_replace = empty
            });
        }
    }
    token_with_ignore_list_vec replaced = replace_macros_helper(tokens_with_ignore_list, 0, macro_map);
    pp_token_vec out;
    pp_token_vec_init(&out, replaced.n_elements);
    for (size_t i = 0; i < replaced.n_elements; i++) {
        if (replaced.arr[i].token.name.n > 0) { // remove placemarkers
            pp_token_vec_append(&out, replaced.arr[i].token);
        }
    }
    return out;
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
                if (info.args[i].tokens[j].token.after_whitespace && j != 0) {
                    printf(" ");
                }
                printf("%.*s", (int)info.args[i].tokens[j].token.name.n, (const char*)info.args[i].tokens[j].token.name.first);
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


