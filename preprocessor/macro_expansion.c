#include "macro_expansion.h"
#include "preprocessor/diagnostics.h"
#include <stdio.h>

static struct earley_rule get_replacement_list_rule(const struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &tr_control_line) {
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

static pp_token_vec get_replacement_tokens(const struct earley_rule control_line_rule) {
    const struct earley_rule replacement_list_rule = get_replacement_list_rule(control_line_rule);
    const struct earley_rule pp_tokens_opt_rule = *replacement_list_rule.completed_from.arr[0];
    pp_token_vec replacement_tokens = pp_token_vec_new(0);
    if (pp_tokens_opt_rule.rhs.tag == OPT_ONE) {
        const struct earley_rule pp_tokens_rule = *pp_tokens_opt_rule.completed_from.arr[0];
        for (size_t i = 0; i < pp_tokens_rule.completed_from.n_elements; i++) {
            const struct earley_rule pp_token_rule = *pp_tokens_rule.completed_from.arr[i];
            pp_token_vec_append(&replacement_tokens, pp_token_rule.rhs.symbols[0].val.terminal.token);
        }
    }
    return replacement_tokens;
}

static struct preprocessing_token get_macro_name_token(const struct earley_rule control_line_rule) {
    return control_line_rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token;
}

static bool replacement_lists_identical(const struct preprocessing_token *const list1, const size_t n1, const struct preprocessing_token *const list2, const size_t n2) {
    if (n1 != n2) return false;
    for (size_t i = 0; i < n1; i++) {
        if (!str_views_eq(list1[i].name, list2[i].name)
        || (i != 0 && list1[i].after_whitespace != list2[i].after_whitespace)) {
            return false;
        }
    }
    return true;
}

void define_object_like_macro(const struct earley_rule rule, str_view_macro_args_and_body_map *macros) {
    if (rule.lhs != &tr_control_line || rule.rhs.tag != CONTROL_LINE_DEFINE_OBJECT_LIKE) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }
    const struct preprocessing_token macro_name_token = get_macro_name_token(rule);

    const pp_token_vec replacement_tokens = get_replacement_tokens(rule);

    if (str_view_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        const struct macro_args_and_body existing_macro = str_view_macro_args_and_body_map_get(macros, macro_name_token.name);
        if (!replacement_lists_identical(replacement_tokens.arr, replacement_tokens.n_elements,
                                        existing_macro.replacements, existing_macro.n_replacements)) {
            preprocessor_error(0, 0, 0, "macro already exists");
        }
        return;
    }

    str_view_macro_args_and_body_map_add(macros, macro_name_token.name,
        (struct macro_args_and_body) {
            .is_function_like = false,
            .args = NULL,
            .n_args = 0,
            .accepts_varargs = false,
            .replacements = replacement_tokens.arr,
            .n_replacements = replacement_tokens.n_elements
        }
    );
}

static void append_identifier_names(str_view_vec *vec, const struct earley_rule identifier_list_rule) {
    for (size_t i = 0; i < identifier_list_rule.completed_from.n_elements; i++) {
        const struct earley_rule identifier_rule = *identifier_list_rule.completed_from.arr[i];
        const struct preprocessing_token token = identifier_rule.rhs.symbols[0].val.terminal.token;
        str_view_vec_append(vec, token.name);
    }
}

static str_view_vec get_macro_params(const struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &tr_control_line || (control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    str_view_vec params = str_view_vec_new(0);

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
            const struct earley_rule identifier_list_opt_rule = *control_line_rule.completed_from.arr[2];
            if (identifier_list_opt_rule.rhs.tag == OPT_ONE) {
                const struct earley_rule identifier_list_rule = *identifier_list_opt_rule.completed_from.arr[0];
                append_identifier_names(&params, identifier_list_rule);
            }
            break;
        }
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
            break;
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS: {
            const struct earley_rule identifier_list_rule = *control_line_rule.completed_from.arr[2];
            for (size_t i = 0; i < identifier_list_rule.completed_from.n_elements; i++) {
                const struct earley_rule identifier_rule = *identifier_list_rule.completed_from.arr[i];
                const struct preprocessing_token token = identifier_rule.rhs.symbols[0].val.terminal.token;
                str_view_vec_append(&params, token.name);
            }
            break;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    return params;
}

static bool args_identical(const struct str_view *const args1, size_t n1, const struct str_view *const args2, const size_t n2) {
    if (n1 != n2) return false;
    for (size_t i = 0; i < n1; i++) {
        if (!str_views_eq(args1[i], args2[i])) return false;
    }
    return true;
}

void define_function_like_macro(const struct earley_rule rule, str_view_macro_args_and_body_map *const macros) {
    if (rule.lhs != &tr_control_line || (rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_function_like_macro is not an function-like macro");
    }
    const struct preprocessing_token macro_name_token = rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token;
    const pp_token_vec replacement_tokens = get_replacement_tokens(rule);
    const str_view_vec args = get_macro_params(rule);

    if (str_view_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        const struct macro_args_and_body existing_macro = str_view_macro_args_and_body_map_get(macros, macro_name_token.name);
        const bool replacements_same = replacement_lists_identical(replacement_tokens.arr, replacement_tokens.n_elements,
                                                                  existing_macro.replacements, existing_macro.n_replacements);
        const bool args_same = args_identical(args.arr, args.n_elements, existing_macro.args, existing_macro.n_args);
        if (!replacements_same || !args_same) {
            preprocessor_error(0, 0, 0, "macro already exists");
        }
        return;
    }

    str_view_macro_args_and_body_map_add(macros, macro_name_token.name,
        (struct macro_args_and_body){
            .is_function_like = true,
            .args = args.arr,
            .n_args = args.n_elements,
            .accepts_varargs = rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS,
            .replacements = replacement_tokens.arr,
            .n_replacements = replacement_tokens.n_elements
        }
    );
}

static struct macro_use_info get_macro_use_info(const token_with_ignore_list_vec tokens, const size_t macro_inv_start, const macro_args_and_body macro_def) {
    const str_view_vec new_dont_replace = str_view_vec_copy(tokens.arr[macro_inv_start].dont_replace);

    if (macro_inv_start == tokens.n_elements - 1 || !token_is_str(tokens.arr[macro_inv_start + 1].token, "(")) {
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

    given_macro_arg_vec given_args = given_macro_arg_vec_new(0);
    token_with_ignore_list_vec current_arg = token_with_ignore_list_vec_new(0);
    token_with_ignore_list_vec varargs = token_with_ignore_list_vec_new(0);
    bool in_varargs = macro_def.accepts_varargs && macro_def.n_args == 0;
    int net_open_parens = 1;
    size_t i = macro_inv_start + 2; // skip the macro name and first open paren
    for (; i < tokens.n_elements; i++) {
        if (token_is_str(tokens.arr[i].token, "(")) {
            net_open_parens++;
        } else if (token_is_str(tokens.arr[i].token, ")")) {
            net_open_parens--;
            if (net_open_parens == 0) {
                i++;
                break;
            }
        } else if (in_varargs) {
            token_with_ignore_list_vec_append(&varargs, tokens.arr[i]);
        }
        if (net_open_parens == 1 && token_is_str(tokens.arr[i].token, ",")) {
            // argument-separating comma
            given_macro_arg_vec_append(&given_args, (struct given_macro_arg) {
                .tokens = current_arg.arr,
                .n_tokens = current_arg.n_elements
            });
            if (macro_def.accepts_varargs && given_args.n_elements == macro_def.n_args) {
                in_varargs = true;
            }
            current_arg = token_with_ignore_list_vec_new(0);
        } else {
            token_with_ignore_list_vec_append(&current_arg, tokens.arr[i]);
        }
    }
    if (net_open_parens > 0) {
        preprocessor_fatal_error(0, 0, 0, "%d open parens in macro call are unmatched by a closed paren", net_open_parens);
    }

    const bool macro_can_take_one_arg = (macro_def.n_args == 1 && !macro_def.accepts_varargs) || (macro_def.n_args == 0 && macro_def.accepts_varargs);
    if (current_arg.n_elements > 0
        || (i >= 2 && token_is_str(tokens.arr[i-2].token, ","))
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

static struct str_view stringify(const struct given_macro_arg arg) {
    uchar_vec out = uchar_vec_new(2); // at least 2 characters (2 quotes)
    uchar_vec_append(&out, '"');
    for (size_t i = 0; i < arg.n_tokens; i++) {
        if (arg.tokens[i].token.after_whitespace && i != 0) {
            uchar_vec_append(&out, ' ');
        }
        for (size_t j = 0; j < arg.tokens[i].token.name.n; j++) {
            if ((arg.tokens[i].token.type == STRING_LITERAL || arg.tokens[i].token.type == CHARACTER_CONSTANT)
            && (arg.tokens[i].token.name.chars[j] == '\\' || arg.tokens[i].token.name.chars[j] == '"')) {
                uchar_vec_append(&out, '\\');
            }
            uchar_vec_append(&out, arg.tokens[i].token.name.chars[j]);
        }
    }
    uchar_vec_append(&out, '"');
    return (struct str_view) { .chars = out.arr, .n = out.n_elements };
}

static struct str_view concatenate(const struct str_view arg1, const struct str_view arg2) {
    uchar_vec out = uchar_vec_new(arg1.n + arg2.n);
    uchar_vec_append_all_arr(&out, arg1.chars, arg1.n);
    uchar_vec_append_all_arr(&out, arg2.chars, arg2.n);
    return (struct str_view) { .chars = out.arr, .n = out.n_elements };
}

static ssize_t get_arg_index(const struct str_view token_name, const struct macro_args_and_body macro_def) {
    for (size_t i = 0; i < macro_def.n_args; i++) {
        if (str_views_eq(token_name, macro_def.args[i])) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static bool str_view_vec_contains(str_view_vec vec, struct str_view view) {
    for (size_t i = 0; i < vec.n_elements; i++) {
        if (str_views_eq(vec.arr[i], view)) {
            return true;
        }
    }
    return false;
}

static token_with_ignore_list_vec replace_macros_helper(token_with_ignore_list_vec tokens, size_t scan_start, str_view_macro_args_and_body_map macro_map);

static token_with_ignore_list_vec replace_arg(const struct given_macro_arg arg, const str_view_macro_args_and_body_map macro_map, const struct str_view macro_name, const str_view_vec ignore_list) {
    token_with_ignore_list_vec arg_tokens = token_with_ignore_list_vec_new(0);
    for (size_t i = 0; i < arg.n_tokens; i++) {
        str_view_vec new_ignore_list = str_view_vec_copy(ignore_list);
        str_view_vec_append_all(&new_ignore_list, arg.tokens[i].dont_replace);
        token_with_ignore_list_vec_append(&arg_tokens, (struct token_with_ignore_list) {
                .token = arg.tokens[i].token, .dont_replace = new_ignore_list
        });
    }
    const token_with_ignore_list_vec out = replace_macros_helper(arg_tokens, 0, macro_map);
    for (size_t i = 0; i < out.n_elements; i++) {
        str_view_vec_append(&out.arr[i].dont_replace, macro_name);
    }
    return out;
}

static pp_token_vec eval_stringifies(struct macro_args_and_body macro_info, struct macro_use_info use_info) {
    pp_token_vec out = pp_token_vec_new(0);
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
    printf("getting replacement for call of macro %.*s\n", (int)use_info.macro_name.n, use_info.macro_name.chars);

    // TODO error if __VA_ARGS__ is used outside a variadic macro

    if (macro_info.accepts_varargs) {
        // The macro use info comes in with the normal argument and varargs as separate. This adds all the varargs as 1 extra argument at the end.
        given_macro_arg_vec new_given_args = given_macro_arg_vec_new(use_info.n_args + 1);
        given_macro_arg_vec_append_all_arr(&new_given_args, use_info.args, macro_info.n_args); // not a typo
        given_macro_arg_vec_append(&new_given_args,
                                   (struct given_macro_arg) {.tokens = use_info.vararg_tokens, .n_tokens = use_info.n_vararg_tokens});
        use_info.args = new_given_args.arr;
        use_info.n_args = new_given_args.n_elements;

        // Similarly, we need to add an extra argument to the macro info, called __VA_ARGS__
        str_view_vec new_arg_names = str_view_vec_new(macro_info.n_args + 1);
        str_view_vec_append_all_arr(&new_arg_names, macro_info.args, macro_info.n_args);
        str_view_vec_append(&new_arg_names, (struct str_view) {.chars = (const unsigned char *) "__VA_ARGS__", .n =
        sizeof("__VA_ARGS__") - 1});
        macro_info.args = new_arg_names.arr;
        macro_info.n_args = new_arg_names.n_elements;
    }

    token_with_ignore_list_vec replaced_tokens = token_with_ignore_list_vec_new(0);
    boolean_vec needs_concat = boolean_vec_new(0);

    str_view_vec dont_replace_list = str_view_vec_copy(use_info.dont_replace);
    str_view_vec_append(&dont_replace_list, use_info.macro_name);

    /*
     The purpose of the dont_add_left_operand flag is to handle cases like this one:
         #define M(a, b, c) a##b##c
         M(x, y, z)
     The flag indicates that a##b has already been processed, and we are now processing the concatenation b##c; so b should not be added to the tokens list again.
     If it didn't exist, a##b##c would (incorrectly) evaluate to xy yz, because:
       1) First, a##b is processed.
         1.1) Both tokens are replaced and added to the tokens list. So replaced_tokens is now [x, y].
         1.2) x is the left operand, so it's marked as not needing to be concatenated with the preceding token. y is the right operand, so it does need to be concatenated with the preceding token (which is x). So needs_concat is now [false, true].
       2) Then, b##c is processed.
         2.1) Both tokens are replaced and added to the tokens list. So replaced_tokens is now [x, y, y, z].
         2.2) The left operand, y, is marked as not needing to be concatenated with the preceding token. z does need to be concatenated with the preceding token (which is y). So needs_concat is now [false, true, false, true].
       3) Then, the tokens are concatenated according to needs_concat, which concatenates the first y with its preceding token (x) and z with its preceding token (the second y).
       So the final output would be xy yz, which is incorrect.

     With the dont_add_left_operand flag, the flag is set right before step 2, which means b isn't added to replaced_tokens; and the corresponding false isn't added to needs_concat.
     So, at the end of step 2, replaced_tokens is [x, y, z], and needs_concat is [false, true, true]; this produces xyz, which is correct.
    */
    bool dont_add_left_operand = false;

    pp_token_vec stringifies_expanded = eval_stringifies(macro_info, use_info);

    // Token concatenation is evaluated in 2 stages:
    // 1. Every token is macro-replaced appropriately, including the operands to the token concatenation.
    //    A bool vector needs_concat is created; if needs_concat[i] is true then replaced_tokens[i] needs to be concatenated with the token before it.
    // 2. The tokens are concatenated according to needs_concat.
    // This for loop is step 1.
    for (size_t i = 0; i < stringifies_expanded.n_elements;) {
        // If ## is after this token, then...
        if (i != stringifies_expanded.n_elements - 1 && token_is_str(stringifies_expanded.arr[i+1], "##")) {
            if (i+1 == stringifies_expanded.n_elements - 1) {
                preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
            }

            // Figure out whether the operands are macro arguments, and if so, what the indices of the arguments are
            const ssize_t left_operand_arg_i = get_arg_index(stringifies_expanded.arr[i].name, macro_info);
            const ssize_t right_operand_arg_i = get_arg_index(stringifies_expanded.arr[i + 2].name, macro_info);

            if (!dont_add_left_operand) {
                if (left_operand_arg_i == -1) {
                    // If the left operand isn't an argument, it doesn't need to be replaced. Add it to the tokens list.
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = stringifies_expanded.arr[i], .dont_replace = str_view_vec_copy(dont_replace_list)
                    });
                    // The left operand shouldn't be concatenated with the token before it
                    boolean_vec_append(&needs_concat, false);
                } else if (use_info.args[left_operand_arg_i].n_tokens == 0) {
                    // If the left operand is an argument, but it was given as the empty string (e.g. the first argument in X(,3)), add a placemarker token.
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = {
                                    .name = {.chars = (const unsigned char *) "", .n = 0},
                                    .after_whitespace = stringifies_expanded.arr[i].after_whitespace,
                                    // type intentionally omitted
                            },
                            .dont_replace = str_view_vec_copy(dont_replace_list)
                    });
                    // The left operand shouldn't be concatenated with the token before it
                    boolean_vec_append(&needs_concat, false);
                } else {
                    // If the left operand is a non-empty argument, add all the tokens from the given argument
                    token_with_ignore_list_vec_append_all_arr(&replaced_tokens,
                                                              use_info.args[left_operand_arg_i].tokens,
                                                              use_info.args[left_operand_arg_i].n_tokens);
                    // None of left operand's tokens should be concatenated with the preceding token
                    for (size_t j = 0; j < use_info.args[left_operand_arg_i].n_tokens; j++) {
                        boolean_vec_append(&needs_concat, false);
                    }
                }
            }
            if (right_operand_arg_i == -1) {
                // Like with the left operand, if the right operand isn't an argument, just add it to the tokens list
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = stringifies_expanded.arr[i+2], .dont_replace = str_view_vec_copy(dont_replace_list)
                });
                // Indicate it should be concatenated with the preceding token (which is the left operand)
                boolean_vec_append(&needs_concat, true);
            } else if (use_info.args[right_operand_arg_i].n_tokens == 0) {
                // Like with the left operand, if the right operand is given as the empty string, add a placemarker token
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = {
                                .name = { .chars = (const unsigned char*)"", .n = 0 },
                                .after_whitespace = stringifies_expanded.arr[i].after_whitespace,
                                // type intentionally omitted
                        },
                        .dont_replace = str_view_vec_copy(dont_replace_list)
                });
                // This placemaker token still needs to be concatenated with the left operand
                boolean_vec_append(&needs_concat, true);
            } else {
                // Like with the left operand, if it's a non-empty argument, we add all the tokens from the given argument
                token_with_ignore_list_vec_append_all_arr(&replaced_tokens, use_info.args[right_operand_arg_i].tokens, use_info.args[right_operand_arg_i].n_tokens);
                // The first token of the right operand's expansion should be concatenated with the left operand
                boolean_vec_append(&needs_concat, true);
                // The rest should not
                for (size_t j = 1; j < use_info.args[right_operand_arg_i].n_tokens; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            dont_add_left_operand = true;
            i += 2;
        } else if (i == 0 && token_is_str(stringifies_expanded.arr[i], "##")) {
            preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
        } else if (dont_add_left_operand) {
            i++;
            dont_add_left_operand = false;
        } else {
            // This is a normal token, i.e. not an operand to the ## operator, and not the ## operator itself; and the same for the # operator, since stringifications were expanded earlier
            // Figure out whether it's a macro argument, and if so, what its index is
            ssize_t arg_index = get_arg_index(stringifies_expanded.arr[i].name, macro_info);
            if (arg_index == -1) {
                // If it's not an argument, just add it to the tokens list
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                    .token = stringifies_expanded.arr[i], .dont_replace = str_view_vec_copy(dont_replace_list) }
                );
                // It isn't the right operand of the ## operator, so it shouldn't be concatenated with the preceding token
                boolean_vec_append(&needs_concat, false);
            } else {
                // If it's an argument, add the given argument's tokens to the list
                token_with_ignore_list_vec new_arg = replace_arg(use_info.args[arg_index], macro_map, use_info.macro_name, use_info.dont_replace);
                token_with_ignore_list_vec_append_all(&replaced_tokens, new_arg);
                // None of its tokens are the right operand of the ## operator, so they shouldn't be concatenated with the preceding token
                for (size_t j = 0; j < new_arg.n_elements; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            dont_add_left_operand = false;
            i++;
        }
    }

    // This for loop is step 2. The tokens are concatenated according to needs_concat.
    token_with_ignore_list_vec out = token_with_ignore_list_vec_new(0);
    for (size_t i = 0; i < replaced_tokens.n_elements; i++) {
        if (needs_concat.arr[i]) {
            struct str_view concat_result = concatenate(out.arr[out.n_elements - 1].token.name, replaced_tokens.arr[i].token.name);
            bool token_valid = is_valid_token(concat_result, EXCLUDE_HEADER_NAME);
            if (!token_valid && concat_result.n != 0) {
                preprocessor_fatal_error(0, 0, 0, "concat result %.*s is not a valid token", (int)concat_result.n, concat_result.chars);
            }
            // If a token needs to be concatenated with the preceding token, then we retroactively modify the preceding token
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
    token_with_ignore_list_vec out = token_with_ignore_list_vec_new(0);

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
            token_with_ignore_list_vec_append_all(&out, replaced_tokens);
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
    token_with_ignore_list_vec tokens_with_ignore_list = token_with_ignore_list_vec_new(tokens.n_elements);
    for (size_t i = 0; i < tokens.n_elements; i++) {
        if (!token_is_str(tokens.arr[i], "\n")) {
            token_with_ignore_list_vec_append(&tokens_with_ignore_list, (struct token_with_ignore_list) {
                    .token = tokens.arr[i],
                    .dont_replace = str_view_vec_new(0)
            });
        }
    }
    token_with_ignore_list_vec replaced = replace_macros_helper(tokens_with_ignore_list, 0, macro_map);
    pp_token_vec out = pp_token_vec_new(replaced.n_elements);
    for (size_t i = 0; i < replaced.n_elements; i++) {
        if (replaced.arr[i].token.name.n > 0) { // remove placemarkers
            pp_token_vec_append(&out, replaced.arr[i].token);
        }
    }
    return out;
}


void reconstruct_macro_use(struct macro_use_info info) {
    // Print the macro name
    printf("Macro use: %.*s", (int)info.macro_name.n, (const char*)info.macro_name.chars);

    // If the macro is function-like and there are arguments, print them within parentheses
    if (info.is_function_like) {
        printf("(");
        for (size_t i = 0; i < info.n_args; i++) {
            // Print each argument
            for (size_t j = 0; j < info.args[i].n_tokens; j++) {
                if (info.args[i].tokens[j].token.after_whitespace && j != 0) {
                    printf(" ");
                }
                printf("%.*s", (int)info.args[i].tokens[j].token.name.n, (const char*)info.args[i].tokens[j].token.name.chars);
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


void print_macros(const str_view_macro_args_and_body_map *const macros) {
    for (size_t i = 0; i < macros->n_buckets; i++) {
        NODE_T(str_view, macro_args_and_body) *node = macros->buckets[i];
        while (node != NULL) {
            printf("Macro: ");
            for (size_t j = 0; j < node->key.n; j++) {
                printf("%c", node->key.chars[j]);
            }

            if (node->value.is_function_like) {
                printf("(");
                for (size_t arg_index = 0; arg_index < node->value.n_args; arg_index++) {
                    for (size_t k = 0; k < node->value.args[arg_index].n; k++) {
                        printf("%c", node->value.args[arg_index].chars[k]);
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
                    printf("%c", token.name.chars[k]);
                }
            }
            printf("\n");
            node = node->next;
        }
    }
}


