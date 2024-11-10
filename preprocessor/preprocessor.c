#include "preprocessor.h"

#include "conditional_inclusion.h"
#include "diagnostics.h"
#include "macro_expansion.h"
#include "debug/color_print.h"

static pp_token_harr preprocess_tree(const struct earley_rule group_opt_rule) {
    pp_token_vec out = pp_token_vec_new(0);
    if (group_opt_rule.rhs.tag == OPT_NONE) {
        return out.arr;
    }

    const struct earley_rule group_rule = *group_opt_rule.completed_from.data[0];
    sstr_macro_args_and_body_map macro_map = sstr_macro_args_and_body_map_new(0);
    pp_token_vec text_section = pp_token_vec_new(0);
    for (size_t i = 0; i < group_rule.completed_from.len; i++) {
        const struct earley_rule group_part_rule = *group_rule.completed_from.data[i];
        if (group_part_rule.rhs.tag != GROUP_PART_TEXT) {
            pp_token_vec_append_all_harr(&out, replace_macros(text_section.arr, macro_map));
            text_section.arr.len = 0;  // TODO replace with call to shrink_retaining_capacity
        }
        switch ((enum group_part_tag)group_part_rule.rhs.tag) {
            case GROUP_PART_CONTROL: {
                const struct earley_rule control_line_rule = *group_part_rule.completed_from.data[0];
                switch ((enum control_line_tag)control_line_rule.rhs.tag) {
                    case CONTROL_LINE_DEFINE_OBJECT_LIKE: {
                        define_object_like_macro(control_line_rule, &macro_map);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(&macro_map);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
                        define_function_like_macro(control_line_rule, &macro_map);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(&macro_map);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_UNDEF: {
                        // Removes the macro if it exists; does nothing if it doesn't
                        sstr_macro_args_and_body_map_remove(&macro_map, control_line_rule.completed_from.data[0]->rhs.symbols.data[0].val.terminal.token.name);
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
                struct earley_rule *const included_group_opt = eval_if_section(if_section_rule, macro_map);
                if (included_group_opt) {
                    pp_token_vec_append_all_harr(&out, preprocess_tree(*included_group_opt));
                }
                break;
            }
            case GROUP_PART_NON_DIRECTIVE:
                preprocessor_fatal_error(0, 0, 0, "invalid directive");
        }
    }
    pp_token_vec_append_all_harr(&out, replace_macros(text_section.arr, macro_map));
    return out.arr;
}

pp_token_harr preprocess_full_file_tree(const struct earley_rule *const root) {
    return preprocess_tree(*root->completed_from.data[0]);
}
