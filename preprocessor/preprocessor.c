#include "preprocessor.h"

#include "conditional_inclusion.h"
#include "diagnostics.h"
#include "escaped_newlines.h"
#include "macro_expansion.h"
#include "trigraphs.h"
#include "debug/color_print.h"
#include "driver/file_utils.h"

static pp_token_harr preprocess_included_file(FILE *input_file, sstr_macro_args_and_body_map *macro_map);

static pp_token_harr preprocess_tree(const struct earley_rule group_opt_rule, sstr_macro_args_and_body_map *macro_map) {
    pp_token_vec out = pp_token_vec_new(0);
    if (group_opt_rule.rhs.tag == OPT_NONE) {
        return out.arr;
    }

    const struct earley_rule group_rule = *group_opt_rule.completed_from.data[0];

    pp_token_vec text_section = pp_token_vec_new(0);
    for (size_t i = 0; i < group_rule.completed_from.len; i++) {
        const struct earley_rule group_part_rule = *group_rule.completed_from.data[i];
        if (group_part_rule.rhs.tag != GROUP_PART_TEXT) {
            pp_token_vec_append_all_harr(&out, replace_macros(text_section.arr, *macro_map, EXCLUDE_HEADER_NAME));
            text_section.arr.len = 0;  // TODO replace with call to shrink_retaining_capacity
        }
        switch ((enum group_part_tag)group_part_rule.rhs.tag) {
            case GROUP_PART_CONTROL: {
                const struct earley_rule control_line_rule = *group_part_rule.completed_from.data[0];
                switch ((enum control_line_tag)control_line_rule.rhs.tag) {
                    case CONTROL_LINE_DEFINE_OBJECT_LIKE: {
                        define_object_like_macro(control_line_rule, macro_map);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(macro_map);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
                        define_function_like_macro(control_line_rule, macro_map);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(macro_map);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_UNDEF: {
                        // Removes the macro if it exists; does nothing if it doesn't
                        sstr_macro_args_and_body_map_remove(macro_map, control_line_rule.completed_from.data[0]->rhs.symbols.data[0].val.terminal.token.name);
                        break;
                    }
                    case CONTROL_LINE_INCLUDE: {
                        const struct earley_rule pp_tokens_rule = *control_line_rule.completed_from.data[0];
                        const pp_token_harr initial_arg_tokens = replace_macros(pp_tokens_rule_as_harr(pp_tokens_rule), *macro_map, EXCLUDE_STRING_LITERAL);
                        uchar_vec chars_to_retokenize = uchar_vec_new(0);
                        for (size_t j = 0; j < initial_arg_tokens.len; j++) {
                            if (initial_arg_tokens.data[j].after_whitespace) {
                                uchar_vec_append(&chars_to_retokenize, ' ');
                            }
                            uchar_vec_append_all_harr(&chars_to_retokenize, initial_arg_tokens.data[j].name);
                        }
                        const pp_token_harr retokenized_arg = get_pp_tokens(chars_to_retokenize.arr, true);

                        if (retokenized_arg.len != 1) {
                            preprocessor_fatal_error(0, 0, 0, "#include directive expects one argument");
                        }
                        const struct preprocessing_token arg_token = retokenized_arg.data[0];
                        if (arg_token.type != HEADER_NAME && arg_token.type != STRING_LITERAL) {
                            preprocessor_fatal_error(0, 0, 0, "Invalid argument to #include directive");
                        }
                        const sstr filename_sstr = slice(arg_token.name, 1, arg_token.name.len - 1); // remove single quotes or angle brackets
                        char *include_filename = MALLOC(filename_sstr.len + 1);
                        memcpy(include_filename, filename_sstr.data, filename_sstr.len);
                        include_filename[filename_sstr.len] = '\0';
                        FILE *include_file = fopen(include_filename, "r");
                        if (include_file == NULL) {
                            preprocessor_fatal_error(0, 0, 0, "Included file \"%s\" does not exist.", include_filename);
                        }
                        pp_token_harr tokens_from_file = preprocess_included_file(include_file, macro_map);
                        if (tokens_from_file.len > 0) {
                            // To make sure it doesn't get smushed with what came before it
                            tokens_from_file.data[0].after_whitespace = true;
                        }
                        pp_token_vec_append_all_harr(&out, tokens_from_file);
                        break;
                    }
                }
                break;
            }
            case GROUP_PART_TEXT: {
                const struct earley_rule text_line_rule = *group_part_rule.completed_from.data[0];
                pp_token_vec text_line_tokens = pp_token_vec_new(0);

                const struct earley_rule *const tokens_not_starting_with_hashtag_opt_rule = text_line_rule.completed_from.data[0];
                if (tokens_not_starting_with_hashtag_opt_rule->rhs.tag == OPT_NONE) {
                    break;
                }
                const struct earley_rule *const tokens_not_starting_with_hashtag_rule = tokens_not_starting_with_hashtag_opt_rule->completed_from.data[0];
                for (size_t j = 0; j < tokens_not_starting_with_hashtag_rule->completed_from.len; j++) {
                    const struct earley_rule *const non_hashtag_rule = tokens_not_starting_with_hashtag_rule->completed_from.data[j];
                    pp_token_vec_append(&text_line_tokens, non_hashtag_rule->rhs.symbols.data[0].val.terminal.token);
                }

                pp_token_vec_append_all_harr(&text_section, text_line_tokens.arr);
                break;
            }
            case GROUP_PART_IF: {
                const struct earley_rule if_section_rule = *group_part_rule.completed_from.data[0];
                struct earley_rule *const included_group_opt = eval_if_section(if_section_rule, *macro_map);
                if (included_group_opt) {
                    pp_token_vec_append_all_harr(&out, preprocess_tree(*included_group_opt, macro_map));
                }
                break;
            }
            case GROUP_PART_NON_DIRECTIVE:
                preprocessor_fatal_error(0, 0, 0, "invalid directive");
        }
    }
    pp_token_vec_append_all_harr(&out, replace_macros(text_section.arr, *macro_map, EXCLUDE_HEADER_NAME));
    return out.arr;
}

static pp_token_harr preprocess_included_file(FILE *input_file, sstr_macro_args_and_body_map *macro_map) {
    const size_t input_len = get_filesize(input_file);
    unsigned char *input_chars = MALLOC(input_len+1);
    fread(input_chars, sizeof(unsigned char), input_len, input_file);
    fclose(input_file);
    input_chars[input_len] = '\n'; // too much of a pain without this

    const struct trigraph_replacement_info trigraph_replacement = replace_trigraphs(
            (sstr){ .data = input_chars, .len = input_len + 1 }
    );
    const struct escaped_newlines_replacement_info logical_lines = rm_escaped_newlines(trigraph_replacement.result);
    const pp_token_harr tokens = get_pp_tokens(logical_lines.result, false);
    const struct earley_rule *const parse_root = parse_full_file(tokens);
    if (parse_root == NULL) {
        preprocessor_fatal_error(0, 0, 0, "Parsing failed");
    }
    const struct earley_rule group_opt_rule = *parse_root->completed_from.data[0];
    return preprocess_tree(group_opt_rule, macro_map);
}

pp_token_harr preprocess_file(FILE *input_file) {
    sstr_macro_args_and_body_map macro_map = sstr_macro_args_and_body_map_new(0);
    return preprocess_included_file(input_file, &macro_map);
}
