#include "macro_expansion.h"
#include "preprocessor/diagnostics.h"
#include <stdio.h>

static struct earley_rule get_replacement_list_rule(const struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &tr_control_line) {
        preprocessor_fatal_error(0, 0, 0, "get_replacement_list_rule called with non-control line");
    }

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_OBJECT_LIKE:
            return *control_line_rule.completed_from.data[1];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS:
            return *control_line_rule.completed_from.data[3];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
            return *control_line_rule.completed_from.data[2];
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS:
            return *control_line_rule.completed_from.data[3];
        default:
            preprocessor_fatal_error(0, 0, 0, "get_replacement_list_rule called with non-define control line");
    }
}

static pp_token_vec get_replacement_tokens(const struct earley_rule control_line_rule) {
    const struct earley_rule replacement_list_rule = get_replacement_list_rule(control_line_rule);
    const struct earley_rule pp_tokens_opt_rule = *replacement_list_rule.completed_from.data[0];
    pp_token_vec replacement_tokens = pp_token_vec_new(0);
    if (pp_tokens_opt_rule.rhs.tag == OPT_ONE) {
        const struct earley_rule pp_tokens_rule = *pp_tokens_opt_rule.completed_from.data[0];
        for (size_t i = 0; i < pp_tokens_rule.completed_from.len; i++) {
            const struct earley_rule pp_token_rule = *pp_tokens_rule.completed_from.data[i];
            pp_token_vec_append(&replacement_tokens, pp_token_rule.rhs.symbols.data[0].val.terminal.token);
        }
    }
    return replacement_tokens;
}

static struct preprocessing_token get_macro_name_token(const struct earley_rule control_line_rule) {
    return control_line_rule.completed_from.data[0]->rhs.symbols.data[0].val.terminal.token;
}

static bool replacement_lists_identical(const pp_token_harr list1, const pp_token_harr list2) {
    if (list1.len != list2.len) return false;
    for (size_t i = 0; i < list1.len; i++) {
        if (!sstrs_eq(list1.data[i].name, list2.data[i].name)
        || (i != 0 && list1.data[i].after_whitespace != list2.data[i].after_whitespace)) {
            return false;
        }
    }
    return true;
}

void define_object_like_macro(const struct earley_rule rule, sstr_macro_args_and_body_map *macros) {
    if (rule.lhs != &tr_control_line || rule.rhs.tag != CONTROL_LINE_DEFINE_OBJECT_LIKE) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }
    const struct preprocessing_token macro_name_token = get_macro_name_token(rule);

    const pp_token_vec replacement_tokens = get_replacement_tokens(rule);

    if (sstr_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        const struct macro_args_and_body existing_macro = sstr_macro_args_and_body_map_get(macros, macro_name_token.name);
        if (!replacement_lists_identical(replacement_tokens.arr, existing_macro.replacements)) {
            preprocessor_error(0, 0, 0, "macro already exists");
        }
        return;
    }

    sstr_macro_args_and_body_map_add(macros, macro_name_token.name,
        (struct macro_args_and_body) {
            .is_function_like = false,
            .args = {.data = NULL, .len = 0},
            .accepts_varargs = false,
            .replacements = replacement_tokens.arr
        }
    );
}

static void append_identifier_names(sstr_vec *vec, const struct earley_rule identifier_list_rule) {
    for (size_t i = 0; i < identifier_list_rule.completed_from.len; i++) {
        const struct earley_rule identifier_rule = *identifier_list_rule.completed_from.data[i];
        const struct preprocessing_token token = identifier_rule.rhs.symbols.data[0].val.terminal.token;
        sstr_vec_append(vec, token.name);
    }
}

static sstr_vec get_macro_params(const struct earley_rule control_line_rule) {
    if (control_line_rule.lhs != &tr_control_line || (control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && control_line_rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    sstr_vec params = sstr_vec_new(0);

    switch (control_line_rule.rhs.tag) {
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
            const struct earley_rule identifier_list_opt_rule = *control_line_rule.completed_from.data[2];
            if (identifier_list_opt_rule.rhs.tag == OPT_ONE) {
                const struct earley_rule identifier_list_rule = *identifier_list_opt_rule.completed_from.data[0];
                append_identifier_names(&params, identifier_list_rule);
            }
            break;
        }
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
            break;
        case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS: {
            const struct earley_rule identifier_list_rule = *control_line_rule.completed_from.data[2];
            for (size_t i = 0; i < identifier_list_rule.completed_from.len; i++) {
                const struct earley_rule identifier_rule = *identifier_list_rule.completed_from.data[i];
                const struct preprocessing_token token = identifier_rule.rhs.symbols.data[0].val.terminal.token;
                sstr_vec_append(&params, token.name);
            }
            break;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "rule passed to define_object_like_macro is not an object-like macro");
    }

    return params;
}

static bool args_identical(sstr_harr args1, sstr_harr args2) {
    if (args1.len != args2.len) return false;
    for (size_t i = 0; i < args1.len; i++) {
        if (!sstrs_eq(args1.data[i], args2.data[i])) return false;
    }
    return true;
}

void define_function_like_macro(const struct earley_rule rule, sstr_macro_args_and_body_map *const macros) {
    if (rule.lhs != &tr_control_line || (rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS && rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS)) {
        preprocessor_fatal_error(0, 0, 0, "rule passed to define_function_like_macro is not an function-like macro");
    }
    const struct preprocessing_token macro_name_token = rule.completed_from.data[0]->rhs.symbols.data[0].val.terminal.token;
    const pp_token_vec replacement_tokens = get_replacement_tokens(rule);
    const sstr_vec args = get_macro_params(rule);

    if (sstr_macro_args_and_body_map_contains(macros, macro_name_token.name)) {
        const struct macro_args_and_body existing_macro = sstr_macro_args_and_body_map_get(macros, macro_name_token.name);
        const bool replacements_same = replacement_lists_identical(replacement_tokens.arr, existing_macro.replacements);
        const bool args_same = args_identical(args.arr, existing_macro.args);
        if (!replacements_same || !args_same) {
            preprocessor_error(0, 0, 0, "macro already exists");
        }
        return;
    }

    sstr_macro_args_and_body_map_add(macros, macro_name_token.name,
        (struct macro_args_and_body){
            .is_function_like = true,
            .args = args.arr,
            .accepts_varargs = rule.rhs.tag != CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS,
            .replacements = replacement_tokens.arr
        }
    );
}

static struct macro_use_info get_macro_use_info(const token_with_ignore_list_vec tokens, const size_t macro_inv_start, const macro_args_and_body macro_def) {
    const sstr_vec new_dont_replace = sstr_vec_copy(tokens.arr.data[macro_inv_start].dont_replace);

    if (macro_inv_start == tokens.arr.len - 1 || !token_is_str(tokens.arr.data[macro_inv_start + 1].token, "(")) {
        // object-like use
        if (macro_def.is_function_like) {
            // object-like use of function-like macro
            return (struct macro_use_info) {
                .is_valid = false
            };
        }
        // object-like use of object-like macro
        return (struct macro_use_info) {
            .macro_name = tokens.arr.data[macro_inv_start].token.name,
            .end_index = macro_inv_start + 1,
            .args = {.data = NULL, .len = 0},
            .is_function_like = false,
            .dont_replace = new_dont_replace,
            .is_valid = true
        };
    } else if (!macro_def.is_function_like) {
        // function-like use of object-like macro
        return (struct macro_use_info) {
                .macro_name = tokens.arr.data[macro_inv_start].token.name,
                .end_index = macro_inv_start + 1,
                .args = {.data = NULL, .len = 0},
                .is_function_like = false,
                .dont_replace = new_dont_replace,
                .is_valid = true
        };
    }
    // function-like use of function-like macro

    token_with_ignore_list_harr_vec given_args = token_with_ignore_list_harr_vec_new(0);
    token_with_ignore_list_vec current_arg = token_with_ignore_list_vec_new(0);
    token_with_ignore_list_vec varargs = token_with_ignore_list_vec_new(0);
    bool in_varargs = macro_def.accepts_varargs && macro_def.args.len == 0;
    int net_open_parens = 1;
    size_t i = macro_inv_start + 2; // skip the macro name and first open paren
    for (; i < tokens.arr.len; i++) {
        if (token_is_str(tokens.arr.data[i].token, "(")) {
            net_open_parens++;
        } else if (token_is_str(tokens.arr.data[i].token, ")")) {
            net_open_parens--;
            if (net_open_parens == 0) {
                i++;
                break;
            }
        } else if (in_varargs) {
            token_with_ignore_list_vec_append(&varargs, tokens.arr.data[i]);
        }
        if (net_open_parens == 1 && token_is_str(tokens.arr.data[i].token, ",")) {
            // argument-separating comma
            token_with_ignore_list_harr_vec_append(&given_args, current_arg.arr);
            if (macro_def.accepts_varargs && given_args.arr.len == macro_def.args.len) {
                in_varargs = true;
            }
            current_arg = token_with_ignore_list_vec_new(0);
        } else {
            token_with_ignore_list_vec_append(&current_arg, tokens.arr.data[i]);
        }
    }
    if (net_open_parens > 0) {
        preprocessor_fatal_error(0, 0, 0, "%d open parens in macro call are unmatched by a closed paren", net_open_parens);
    }

    const bool macro_can_take_one_arg = (macro_def.args.len == 1 && !macro_def.accepts_varargs) || (macro_def.args.len == 0 && macro_def.accepts_varargs);
    if (current_arg.arr.len > 0
        || (i >= 2 && token_is_str(tokens.arr.data[i-2].token, ","))
        || (current_arg.arr.len == 0 && given_args.arr.len == 0 && macro_can_take_one_arg)) {
        // current_arg.arr.len > 0: non-empty arg at end, e.g. A(x, y)
        // i >= 2 && token_is_str(tokens.arr.data[i-2], ",")): empty arg at end, e.g. A(x,)
        // current_arg.arr.len == 0 && given_args.arr.len == 0 && macro_can_take_one_arg: handles empty parens (e.g. X()) for definitions like `#define X()` and `#define X(...)`
        token_with_ignore_list_harr_vec_append(&given_args, current_arg.arr);
    }

    if (!macro_def.accepts_varargs && given_args.arr.len != macro_def.args.len) {
        preprocessor_fatal_error(0, 0, 0,
            "wrong number of args given to macro; expected %zu, got %zu", macro_def.args.len, given_args.arr.len);
    } else if (macro_def.accepts_varargs && given_args.arr.len < macro_def.args.len + 1) {
        preprocessor_fatal_error(0, 0, 0,
            "too few args given to varargs macro; expected at least %zu, got %zu", macro_def.args.len + 1, given_args.arr.len);
    }

    return (struct macro_use_info) {
        .macro_name = tokens.arr.data[macro_inv_start].token.name,
        .end_index = i,
        .args = given_args.arr,
        .vararg_tokens = varargs.arr,
        .is_function_like = true,
        .dont_replace = new_dont_replace,
        .is_valid = true
    };
}

static sstr stringify(const token_with_ignore_list_harr arg) {
    uchar_vec out = uchar_vec_new(2); // at least 2 characters (2 quotes)
    uchar_vec_append(&out, '"');
    for (size_t i = 0; i < arg.len; i++) {
        if (arg.data[i].token.after_whitespace && i != 0) {
            uchar_vec_append(&out, ' ');
        }
        for (size_t j = 0; j < arg.data[i].token.name.len; j++) {
            if ((arg.data[i].token.type == STRING_LITERAL || arg.data[i].token.type == CHARACTER_CONSTANT)
            && (arg.data[i].token.name.data[j] == '\\' || arg.data[i].token.name.data[j] == '"')) {
                uchar_vec_append(&out, '\\');
            }
            uchar_vec_append(&out, arg.data[i].token.name.data[j]);
        }
    }
    uchar_vec_append(&out, '"');
    return out.arr;
}

static sstr concatenate(const sstr arg1, const sstr arg2) {
    uchar_vec out = uchar_vec_new(arg1.len + arg2.len);
    uchar_vec_append_all_arr(&out, arg1.data, arg1.len);
    uchar_vec_append_all_arr(&out, arg2.data, arg2.len);
    return out.arr;
}

static ssize_t get_arg_index(const sstr token_name, const struct macro_args_and_body macro_def) {
    for (size_t i = 0; i < macro_def.args.len; i++) {
        if (sstrs_eq(token_name, macro_def.args.data[i])) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static bool sstr_vec_contains(sstr_vec vec, sstr view) {
    for (size_t i = 0; i < vec.arr.len; i++) {
        if (sstrs_eq(vec.arr.data[i], view)) {
            return true;
        }
    }
    return false;
}

static token_with_ignore_list_vec replace_macros_helper(token_with_ignore_list_vec tokens, size_t scan_start, sstr_macro_args_and_body_map macro_map);

static token_with_ignore_list_vec replace_arg(const token_with_ignore_list_harr arg, const sstr_macro_args_and_body_map macro_map, const sstr macro_name, const sstr_vec ignore_list) {
    token_with_ignore_list_vec arg_tokens = token_with_ignore_list_vec_new(0);
    for (size_t i = 0; i < arg.len; i++) {
        sstr_vec new_ignore_list = sstr_vec_copy(ignore_list);
        sstr_vec_append_all(&new_ignore_list, arg.data[i].dont_replace);
        token_with_ignore_list_vec_append(&arg_tokens, (struct token_with_ignore_list) {
                .token = arg.data[i].token, .dont_replace = new_ignore_list
        });
    }
    const token_with_ignore_list_vec out = replace_macros_helper(arg_tokens, 0, macro_map);
    for (size_t i = 0; i < out.arr.len; i++) {
        sstr_vec_append(&out.arr.data[i].dont_replace, macro_name);
    }
    return out;
}

static pp_token_vec eval_stringifies(const struct macro_args_and_body macro_info, const struct macro_use_info use_info) {
    pp_token_vec out = pp_token_vec_new(0);
    for (size_t i = 0; i < macro_info.replacements.len;) {
        if (i != macro_info.replacements.len - 1 && token_is_str(macro_info.replacements.data[i], "#") && macro_info.is_function_like) {
            ssize_t arg_index = get_arg_index(macro_info.replacements.data[i+1].name, macro_info);
            if (arg_index == -1) {
                preprocessor_fatal_error(0, 0, 0, "can't stringify non-argument");
            }

            pp_token_vec_append(&out, (struct preprocessing_token){
                            .name = stringify(use_info.args.data[arg_index]),
                            .type=STRING_LITERAL,
                            .after_whitespace = macro_info.replacements.data[i].after_whitespace
                    });
            i += 2;
        } else if (i == macro_info.replacements.len - 1 && token_is_str(macro_info.replacements.data[i], "#") && macro_info.is_function_like) {
            preprocessor_fatal_error(0, 0, 0, "# operator can't appear at the end of a macro");
        }
        else {
            pp_token_vec_append(&out, macro_info.replacements.data[i]);
            i++;
        }
    }
    return out;
}

typedef _Bool boolean;
DEFINE_VEC_TYPE_AND_FUNCTIONS(boolean)

static token_with_ignore_list_vec get_replacement(struct macro_args_and_body macro_info, struct macro_use_info use_info, sstr_macro_args_and_body_map macro_map) {
    printf("getting replacement for call of macro %.*s\n", (int)use_info.macro_name.len, use_info.macro_name.data);

    // TODO error if __VA_ARGS__ is used outside a variadic macro

    if (macro_info.accepts_varargs) {
        // The macro use info comes in with the normal argument and varargs as separate. This adds all the varargs as 1 extra argument at the end.
        token_with_ignore_list_harr_vec new_given_args = token_with_ignore_list_harr_vec_new(use_info.args.len + 1);
        token_with_ignore_list_harr_vec_append_all_arr(&new_given_args, use_info.args.data, macro_info.args.len); // not a typo
        token_with_ignore_list_harr_vec_append(&new_given_args, use_info.vararg_tokens);
        use_info.args = new_given_args.arr;

        // Similarly, we need to add an extra argument to the macro info, called __VA_ARGS__
        sstr_vec new_arg_names = sstr_vec_new(macro_info.args.len + 1);
        sstr_vec_append_all_arr(&new_arg_names, macro_info.args.data, macro_info.args.len);
        sstr_vec_append(&new_arg_names, SSTR_FROM_LITERAL("__VA_ARGS__"));
        macro_info.args = new_arg_names.arr;
    }

    token_with_ignore_list_vec replaced_tokens = token_with_ignore_list_vec_new(0);
    boolean_vec needs_concat = boolean_vec_new(0);

    sstr_vec dont_replace_list = sstr_vec_copy(use_info.dont_replace);
    sstr_vec_append(&dont_replace_list, use_info.macro_name);

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
    for (size_t i = 0; i < stringifies_expanded.arr.len;) {
        // If ## is after this token, then...
        if (i != stringifies_expanded.arr.len - 1 && token_is_str(stringifies_expanded.arr.data[i+1], "##")) {
            if (i+1 == stringifies_expanded.arr.len - 1) {
                preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
            }

            // Figure out whether the operands are macro arguments, and if so, what the indices of the arguments are
            const ssize_t left_operand_arg_i = get_arg_index(stringifies_expanded.arr.data[i].name, macro_info);
            const ssize_t right_operand_arg_i = get_arg_index(stringifies_expanded.arr.data[i + 2].name, macro_info);

            if (!dont_add_left_operand) {
                if (left_operand_arg_i == -1) {
                    // If the left operand isn't an argument, it doesn't need to be replaced. Add it to the tokens list.
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = stringifies_expanded.arr.data[i], .dont_replace = sstr_vec_copy(dont_replace_list)
                    });
                    // The left operand shouldn't be concatenated with the token before it
                    boolean_vec_append(&needs_concat, false);
                } else if (use_info.args.data[left_operand_arg_i].len == 0) {
                    // If the left operand is an argument, but it was given as the empty string (e.g. the first argument in X(,3)), add a placemarker token.
                    token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                            .token = {
                                    .name = SSTR_FROM_LITERAL(""),
                                    .after_whitespace = stringifies_expanded.arr.data[i].after_whitespace,
                                    // type intentionally omitted
                            },
                            .dont_replace = sstr_vec_copy(dont_replace_list)
                    });
                    // The left operand shouldn't be concatenated with the token before it
                    boolean_vec_append(&needs_concat, false);
                } else {
                    // If the left operand is a non-empty argument, add all the tokens from the given argument
                    token_with_ignore_list_vec_append_all_arr(&replaced_tokens,
                                                              use_info.args.data[left_operand_arg_i].data,
                                                              use_info.args.data[left_operand_arg_i].len);
                    // None of left operand's tokens should be concatenated with the preceding token
                    for (size_t j = 0; j < use_info.args.data[left_operand_arg_i].len; j++) {
                        boolean_vec_append(&needs_concat, false);
                    }
                }
            }
            if (right_operand_arg_i == -1) {
                // Like with the left operand, if the right operand isn't an argument, just add it to the tokens list
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = stringifies_expanded.arr.data[i+2], .dont_replace = sstr_vec_copy(dont_replace_list)
                });
                // Indicate it should be concatenated with the preceding token (which is the left operand)
                boolean_vec_append(&needs_concat, true);
            } else if (use_info.args.data[right_operand_arg_i].len == 0) {
                // Like with the left operand, if the right operand is given as the empty string, add a placemarker token
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                        .token = {
                                .name = SSTR_FROM_LITERAL(""),
                                .after_whitespace = stringifies_expanded.arr.data[i].after_whitespace,
                                // type intentionally omitted
                        },
                        .dont_replace = sstr_vec_copy(dont_replace_list)
                });
                // This placemarker token still needs to be concatenated with the left operand
                boolean_vec_append(&needs_concat, true);
            } else {
                // Like with the left operand, if it's a non-empty argument, we add all the tokens from the given argument
                token_with_ignore_list_vec_append_all_arr(&replaced_tokens, use_info.args.data[right_operand_arg_i].data, use_info.args.data[right_operand_arg_i].len);
                // The first token of the right operand's expansion should be concatenated with the left operand
                boolean_vec_append(&needs_concat, true);
                // The rest should not
                for (size_t j = 1; j < use_info.args.data[right_operand_arg_i].len; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            dont_add_left_operand = true;
            i += 2;
        } else if (i == 0 && token_is_str(stringifies_expanded.arr.data[i], "##")) {
            preprocessor_fatal_error(0, 0, 0, "## can't appear at beginning or end of macro");
        } else if (dont_add_left_operand) {
            i++;
            dont_add_left_operand = false;
        } else {
            // This is a normal token, i.e. not an operand to the ## operator, and not the ## operator itself; and the same for the # operator, since stringifications were expanded earlier
            // Figure out whether it's a macro argument, and if so, what its index is
            ssize_t arg_index = get_arg_index(stringifies_expanded.arr.data[i].name, macro_info);
            if (arg_index == -1) {
                // If it's not an argument, just add it to the tokens list
                token_with_ignore_list_vec_append(&replaced_tokens, (struct token_with_ignore_list) {
                    .token = stringifies_expanded.arr.data[i], .dont_replace = sstr_vec_copy(dont_replace_list) }
                );
                // It isn't the right operand of the ## operator, so it shouldn't be concatenated with the preceding token
                boolean_vec_append(&needs_concat, false);
            } else {
                // If it's an argument, add the given argument's tokens to the list
                token_with_ignore_list_vec new_arg = replace_arg(use_info.args.data[arg_index], macro_map, use_info.macro_name, use_info.dont_replace);
                token_with_ignore_list_vec_append_all(&replaced_tokens, new_arg);
                // None of its tokens are the right operand of the ## operator, so they shouldn't be concatenated with the preceding token
                for (size_t j = 0; j < new_arg.arr.len; j++) {
                    boolean_vec_append(&needs_concat, false);
                }
            }
            dont_add_left_operand = false;
            i++;
        }
    }

    // This for loop is step 2. The tokens are concatenated according to needs_concat.
    token_with_ignore_list_vec out = token_with_ignore_list_vec_new(0);
    for (size_t i = 0; i < replaced_tokens.arr.len; i++) {
        if (needs_concat.arr.data[i]) {
            sstr concat_result = concatenate(out.arr.data[out.arr.len - 1].token.name, replaced_tokens.arr.data[i].token.name);
            bool token_valid = is_valid_token(concat_result, EXCLUDE_HEADER_NAME);
            if (!token_valid && concat_result.len != 0) {
                preprocessor_fatal_error(0, 0, 0, "concat result %.*s is not a valid token", (int)concat_result.len, concat_result.data);
            }
            // If a token needs to be concatenated with the preceding token, then we retroactively modify the preceding token
            out.arr.data[out.arr.len - 1].token.name = concat_result;
            if (token_valid) {
                out.arr.data[out.arr.len - 1].token.type = get_token_type_from_str(concat_result, EXCLUDE_HEADER_NAME);
            }
        } else {
            token_with_ignore_list_vec_append(&out, replaced_tokens.arr.data[i]);
        }
    }

    return out;
}

static token_with_ignore_list_vec replace_macros_helper(token_with_ignore_list_vec tokens, size_t scan_start, sstr_macro_args_and_body_map macro_map) {
    token_with_ignore_list_vec out = token_with_ignore_list_vec_new(0);

    bool ignore_replacements = false;
    size_t new_scan_start = tokens.arr.len;
    for (size_t i = 0; i < tokens.arr.len;) {
        if (sstr_macro_args_and_body_map_contains(&macro_map, tokens.arr.data[i].token.name)
        && !ignore_replacements
        && !sstr_vec_contains(tokens.arr.data[i].dont_replace, tokens.arr.data[i].token.name)
        && i >= scan_start) {
            struct macro_args_and_body macro_info = sstr_macro_args_and_body_map_get(&macro_map, tokens.arr.data[i].token.name);
            struct macro_use_info use_info = get_macro_use_info(tokens, i, macro_info);
            if (!use_info.is_valid) {
                token_with_ignore_list_vec_append(&out, tokens.arr.data[i]);
                i++;
                continue;
            }
            token_with_ignore_list_vec replaced_tokens = get_replacement(macro_info, use_info, macro_map);
            token_with_ignore_list_vec_append_all(&out, replaced_tokens);
            new_scan_start = i;
            i = use_info.end_index;
            ignore_replacements = true;
        } else {
            token_with_ignore_list_vec_append(&out, tokens.arr.data[i]);
            i++;
        }
    }
    if (new_scan_start == tokens.arr.len) {
        return out;
    } else {
        return replace_macros_helper(out, new_scan_start, macro_map);
    }
}

pp_token_vec replace_macros(pp_token_vec tokens, sstr_macro_args_and_body_map macro_map) {
    token_with_ignore_list_vec tokens_with_ignore_list = token_with_ignore_list_vec_new(tokens.arr.len);
    for (size_t i = 0; i < tokens.arr.len; i++) {
        if (!token_is_str(tokens.arr.data[i], "\n")) {
            token_with_ignore_list_vec_append(&tokens_with_ignore_list, (struct token_with_ignore_list) {
                    .token = tokens.arr.data[i],
                    .dont_replace = sstr_vec_new(0)
            });
        }
    }
    token_with_ignore_list_vec replaced = replace_macros_helper(tokens_with_ignore_list, 0, macro_map);
    pp_token_vec out = pp_token_vec_new(replaced.arr.len);
    for (size_t i = 0; i < replaced.arr.len; i++) {
        if (replaced.arr.data[i].token.name.len > 0) { // remove placemarkers
            pp_token_vec_append(&out, replaced.arr.data[i].token);
        }
    }
    return out;
}


void reconstruct_macro_use(struct macro_use_info info) {
    // Print the macro name
    printf("Macro use: %.*s", (int)info.macro_name.len, (const char*)info.macro_name.data);

    // If the macro is function-like and there are arguments, print them within parentheses
    if (info.is_function_like) {
        printf("(");
        for (size_t i = 0; i < info.args.len; i++) {
            // Print each argument
            for (size_t j = 0; j < info.args.data[i].len; j++) {
                if (info.args.data[i].data[j].token.after_whitespace && j != 0) {
                    printf(" ");
                }
                printf("%.*s", (int)info.args.data[i].data[j].token.name.len, (const char*)info.args.data[i].data[j].token.name.data);
            }
            if (i < info.args.len - 1) {
                printf(", ");
            }
        }
        printf(")");
        printf(" - Number of arguments given: %zu", info.args.len);
    }
    printf("\n");
}


void print_macros(const sstr_macro_args_and_body_map *const macros) {
    for (size_t i = 0; i < macros->n_buckets; i++) {
        NODE_T(sstr, macro_args_and_body) *node = macros->buckets[i];
        while (node != NULL) {
            printf("Macro: ");
            for (size_t j = 0; j < node->key.len; j++) {
                printf("%c", node->key.data[j]);
            }

            if (node->value.is_function_like) {
                printf("(");
                for (size_t arg_index = 0; arg_index < node->value.args.len; arg_index++) {
                    for (size_t k = 0; k < node->value.args.data[arg_index].len; k++) {
                        printf("%c", node->value.args.data[arg_index].data[k]);
                    }
                    if (arg_index < node->value.args.len - 1 || node->value.accepts_varargs) {
                        printf(", ");
                    }
                }
                if (node->value.accepts_varargs) {
                    printf("...");
                }
                printf(")");
            }

            printf(" -> ");
            for (size_t j = 0; j < node->value.replacements.len; j++) {
                const struct preprocessing_token token = node->value.replacements.data[j];
                if (token.after_whitespace && j != 0) printf(" ");
                for (size_t k = 0; k < token.name.len; k++) {
                    printf("%c", token.name.data[k]);
                }
            }
            printf("\n");
            node = node->next;
        }
    }
}


