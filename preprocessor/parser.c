#include "preprocessor/parser.h"
#include "preprocessor/diagnostics.h"
#include "preprocessor/macro_expansion.h"
#include "preprocessor/conditional_inclusion.h"
#include "data_structures/vector.h"
#include "debug/color_print.h"

static erule_p_harr get_earley_rules(const struct production_rule *const rule, const erule_p_harr *const origin) {
    erule_p_vec out = erule_p_vec_new(rule->alternatives.len);
    for (size_t i = 0; i < rule->alternatives.len; i ++) {
        struct earley_rule *const to_append = MALLOC(sizeof(struct earley_rule));
        *to_append = (struct earley_rule) {
                .lhs=rule, .rhs=rule->alternatives.data[i], .dot=0, .origin_chart=origin, .completed_from={.data = NULL, .len = 0}
        };
        erule_p_vec_append(&out, to_append);
    }
    return out.arr;
}

static void print_rule(struct earley_rule rule);

static struct symbol symbol_after_dot(const struct earley_rule rule) {
    if (rule.dot >= rule.rhs.symbols.len) {
        preprocessor_fatal_error(0, 0, 0, "out of bounds array access in symbol_after_dot");
    }
    return rule.rhs.symbols.data[rule.dot];
}

static erule_p_harr predict(const struct earley_rule rule, const erule_p_harr *const rule_chart) {
    if (rule.dot == rule.rhs.symbols.len || symbol_after_dot(rule).is_terminal) {
        return (erule_p_harr) { .data = NULL, .len = 0 };
    } else {
        const erule_p_harr out = get_earley_rules(symbol_after_dot(rule).val.rule, rule_chart);
        for (size_t i = 0; i < out.len; i++) {
            print_with_color(TEXT_COLOR_LIGHT_BLUE, "{predictor} ");
            print_rule(*out.data[i]);
            print_with_color(TEXT_COLOR_LIGHT_CYAN, " {source:} ");
            print_rule(rule);
            printf("\n");
        }
        return out;
    }
}

static bool rule_is_duplicate(const erule_p_harr rules, const struct earley_rule rule) {
    for (size_t i = 0; i < rules.len; i++) {
        if (rule.lhs == rules.data[i]->lhs // Left hand rule is the same
        && rule.rhs.tag == rules.data[i]->rhs.tag  // Alternative is the same
        && rule.dot == rules.data[i]->dot // Dot is in the same place
        && rule.origin_chart == rules.data[i]->origin_chart // Same origin
        ) {
            return true;
        }
    }
    return false;
}

static void recursively_predict(const struct earley_rule rule, erule_p_vec *const rule_chart) {
    const erule_p_harr first_predictions = predict(rule, &rule_chart->arr);
    for (size_t i = 0; i < first_predictions.len; i++) {
        struct earley_rule *const predict_from = MALLOC(sizeof(struct earley_rule));
        *predict_from = *first_predictions.data[i];
        if (!rule_is_duplicate(rule_chart->arr, *predict_from)) {
            erule_p_vec_append(rule_chart, predict_from);
            recursively_predict(*predict_from, rule_chart);
        }
    }
}

static bool is_completed(const struct earley_rule rule) {
    return rule.dot == rule.rhs.symbols.len;
}

static bool check_terminal(const struct symbol sym, const struct preprocessing_token token) {
    if (sym.is_terminal) {
        switch(sym.val.terminal.type) {
            case TERMINAL_FN:
                return sym.val.terminal.matcher.fn(token);
            case TERMINAL_STR:
                return token_is_str(token, (const char*)sym.val.terminal.matcher.str);
        }
    } else {
        return false;
    }
}

static void complete(struct earley_rule *const rule, erule_p_vec *const out) {
    if (!is_completed(*rule)) {
        return;
    }
    for (size_t i = 0; i < rule->origin_chart->len; i++) {
        const struct earley_rule possible_origin = *rule->origin_chart->data[i];
        if (!is_completed(possible_origin) && !symbol_after_dot(possible_origin).is_terminal && symbol_after_dot(possible_origin).val.rule == rule->lhs) {
            erule_p_vec new_completed_from = erule_p_vec_new(possible_origin.completed_from.len + 1);
            erule_p_vec_append_all_harr(&new_completed_from, possible_origin.completed_from);
            erule_p_vec_append(&new_completed_from, rule);
            struct earley_rule *const to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                    .lhs=possible_origin.lhs, .rhs=possible_origin.rhs, .dot=possible_origin.dot + 1,
                    .origin_chart=possible_origin.origin_chart, .completed_from=new_completed_from.arr
            };
            if (!rule_is_duplicate(out->arr, *to_append)) {
                erule_p_vec_append(out, to_append);
                print_with_color(TEXT_COLOR_LIGHT_GREEN, "{completer} ");
                print_rule(*to_append);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{trigger:} ");
                print_rule(*rule);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{origin:} ");
                print_rule(possible_origin);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{completed from:} ");
                for (size_t j = 0; j < to_append->completed_from.len; j++) {
                    printf("\n\t\t");
                    print_rule(*to_append->completed_from.data[j]);
                }
                printf("\n");
            }
        }
    }
}

static void scan(const struct earley_rule rule, const struct preprocessing_token token, erule_p_vec *const out) {
    if (is_completed(rule) || !symbol_after_dot(rule).is_terminal || !check_terminal(symbol_after_dot(rule), token)) {
        return;
    }
    // Shallow copy the old rule into the new one
    struct earley_rule *const scanned_rule = MALLOC(sizeof(struct earley_rule));
    memcpy(scanned_rule, &rule, sizeof(struct earley_rule));
    // Deep copy the symbols over to the new rule
    scanned_rule->rhs.symbols.data = MALLOC(sizeof(struct symbol) * rule.rhs.symbols.len);
    memcpy(scanned_rule->rhs.symbols.data, rule.rhs.symbols.data, sizeof(struct symbol) * rule.rhs.symbols.len);
    // Make the dot point to the new rule's symbols, in the same position as the old one, then advance it by 1
    scanned_rule->dot = rule.dot + 1;
    // Put the token in the terminal that was scanned over
    scanned_rule->rhs.symbols.data[scanned_rule->dot - 1].val.terminal.token = token;
    // Mark that terminal as containing a token
    scanned_rule->rhs.symbols.data[scanned_rule->dot - 1].val.terminal.is_filled = true;

    print_with_color(TEXT_COLOR_YELLOW, "{scanner} ");
    print_rule(*scanned_rule);
    printf("\n");

    erule_p_vec_append(out, scanned_rule);
}

static erule_p_vec *next_chart(const erule_p_vec *const old_chart, const struct preprocessing_token token) {
    erule_p_vec *const out = MALLOC(sizeof(erule_p_vec));
    *out = erule_p_vec_new(0);
    // Scan
    for (size_t i = 0; i < old_chart->arr.len; i++) {
        scan(*old_chart->arr.data[i], token, out);
    }
    // Complete and predict in a loop
    for (size_t i = 0; i < out->arr.len; i++) {
        struct earley_rule *const rule = out->arr.data[i];
        complete(rule, out);
        recursively_predict(*rule, out);
    }

    return out;
}

static void print_token(struct preprocessing_token token);

static void print_symbol(const struct symbol sym) {

    if (sym.is_terminal) {
        switch (sym.val.terminal.type) {
            case TERMINAL_FN:
                printf("[function] ");
                if (sym.val.terminal.is_filled) {
                    printf("(filled: ");
                    set_color(TEXT_COLOR_GREEN);
                    print_token(sym.val.terminal.token);
                    clear_color();
                    printf(") ");
                }
                break;
            case TERMINAL_STR:
                if (strcmp("\n", (const char *)sym.val.terminal.matcher.str) == 0) {
                    print_with_color(TEXT_COLOR_GREEN, "[newline] ");
                } else {
                    print_with_color(TEXT_COLOR_GREEN, "%s ", (const char*)sym.val.terminal.matcher.str);
                }
                break;
        }
    } else {
        printf("%s ", sym.val.rule->name);
    }
}

static void print_rule(const struct earley_rule rule) {
    printf("%s -> ", rule.lhs->name);
    for (size_t i = 0; i < rule.rhs.symbols.len; i++) {
        if (rule.dot == i) {
            print_with_color(TEXT_COLOR_LIGHT_PURPLE, "• ");
        }
        print_symbol(rule.rhs.symbols.data[i]);
    }
    if (rule.dot == rule.rhs.symbols.len) {
        print_with_color(TEXT_COLOR_LIGHT_PURPLE, "• ");
    }
}

void print_chart(const erule_p_harr *const chart) {
    for (size_t i = 0; i < chart->len; i++) {
        const struct earley_rule rule = *chart->data[i];
        print_rule(rule);
        printf("\n");
    }
}

static void print_token(const struct preprocessing_token token) {
    for (size_t j = 0; j < token.name.len; j++) {
        if (token.name.data[j] == '\n') {
            printf("[newline]");
        } else {
            printf("%c", token.name.data[j]);
        }
    }
}

erule_p_harr_p_harr make_charts(const pp_token_harr tokens, const struct production_rule *const start_rule) {
    erule_p_harr_p_vec out = erule_p_harr_p_vec_new(0);

    erule_p_vec *const initial_chart = MALLOC(sizeof(struct erule_p_vec));
    *initial_chart = erule_p_vec_new(1);
    erule_p_harr_p_vec_append(&out, &initial_chart->arr);

    for (size_t i = 0; i < start_rule->alternatives.len; i++) {
        struct earley_rule *const rule = MALLOC(sizeof(struct earley_rule));
        *rule = (struct earley_rule) {
            .lhs=start_rule, .rhs=start_rule->alternatives.data[i], .dot=0, .origin_chart=&initial_chart->arr /* sketchy */, .completed_from={.data = NULL, .len = 0}
        };
        erule_p_vec_append(initial_chart, rule);
    }

    print_with_color(TEXT_COLOR_LIGHT_RED, "\nInitial Chart:\n");
    print_chart(&initial_chart->arr);

    for (size_t i = 0; i < initial_chart->arr.len; i++) {
        struct earley_rule *const rule = initial_chart->arr.data[i];
        complete(rule, initial_chart);
        recursively_predict(*rule, initial_chart);
    }

    const erule_p_vec *old_chart = initial_chart;
    for (size_t i = 0; i < tokens.len; i++) {
        const struct preprocessing_token token = tokens.data[i];
        print_with_color(TEXT_COLOR_RED, "\nChart after processing token %zu (", i);
        set_color(TEXT_COLOR_GREEN);
        print_token(token);
        clear_color();
        print_with_color(TEXT_COLOR_RED, "):\n");
        erule_p_vec *const new_chart = next_chart(old_chart, tokens.data[i]);
        erule_p_harr_p_vec_append(&out, &new_chart->arr);
        old_chart = new_chart;
    }
    printf("\n");

    return out.arr;
}

static void flatten_list_rules(struct earley_rule *root);
static struct earley_rule *get_tree_root(const erule_p_harr_p_harr charts, const struct production_rule *const root_rule) {
    const erule_p_harr *const final_chart = charts.data[charts.len-1];
    for (size_t i = 0; i < final_chart->len; i++) {
        struct earley_rule *const rule = final_chart->data[i];
        if (rule->lhs == root_rule && is_completed(*rule)) {
            flatten_list_rules(rule);
            return rule;
        }
    }
    return NULL;
}

static erule_p_harr flatten_list_rule(const struct earley_rule list_rule) {
    // would be cleaner if recursive, but would stack overflow for long lists

    erule_p_vec out_reversed = erule_p_vec_new(0);
    struct earley_rule current_rule = list_rule;
    while (current_rule.rhs.tag == LIST_RULE_MULTI) {
        erule_p_vec_append(&out_reversed, current_rule.completed_from.data[1]);
        current_rule = *current_rule.completed_from.data[0];
    }
    erule_p_vec_append(&out_reversed, current_rule.completed_from.data[0]);

    // TODO don't use a vector
    erule_p_vec out = erule_p_vec_new(0);
    for (ssize_t i = (ssize_t)out_reversed.arr.len - 1; i >= 0; i--) {
        erule_p_vec_append(&out, out_reversed.arr.data[i]);
    }
    erule_p_vec_free_internals(&out_reversed);
    return out.arr;
}

static void flatten_list_rules(struct earley_rule *const root) {
    if (root->lhs->is_list_rule) { // TODO autodetect list rules
        const erule_p_harr old_completed_from = root->completed_from;
        root->completed_from = flatten_list_rule(*root);
        FREE(old_completed_from.data);
    }
    for (size_t i = 0; i < root->completed_from.len; i++) {
        flatten_list_rules(root->completed_from.data[i]);
    }
}

void print_tree(const struct earley_rule *const root, const size_t indent) {
    if (root == NULL) {
        print_with_color(TEXT_COLOR_RED, "No tree to print.\n");
        return;
    }
    const enum text_color colors[] = {TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLUE};
    const size_t num_colors = sizeof(colors) / sizeof(colors[0]);
    for (size_t i = 0; i < indent; i++) {
        print_with_color(colors[i % num_colors], ".\t");
    }
    print_rule(*root);
    printf("\n");
    for (size_t i = 0; i < root->completed_from.len; i++) {
        print_tree(root->completed_from.data[i], indent + 1);
    }
}

static pp_token_harr get_text_line_tokens(const struct earley_rule text_line_rule) {
    pp_token_vec text_line_tokens = pp_token_vec_new(0);

    const struct earley_rule *const tokens_not_starting_with_hashtag_opt_rule = text_line_rule.completed_from.data[0];
    if (tokens_not_starting_with_hashtag_opt_rule->rhs.tag == OPT_NONE) {
        return text_line_tokens.arr;
    }
    const struct earley_rule *const tokens_not_starting_with_hashtag_rule = tokens_not_starting_with_hashtag_opt_rule->completed_from.data[0];

    for (size_t i = 0; i < tokens_not_starting_with_hashtag_rule->completed_from.len; i++) {
        const struct earley_rule *const non_hashtag_rule = tokens_not_starting_with_hashtag_rule->completed_from.data[i];
        pp_token_vec_append(&text_line_tokens, non_hashtag_rule->rhs.symbols.data[0].val.terminal.token);
    }

    return text_line_tokens.arr;
}

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
                const pp_token_harr text_line_tokens = get_text_line_tokens(text_line_rule);
                pp_token_vec_append_all_harr(&text_section, text_line_tokens);
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

struct earley_rule *parse(const pp_token_harr tokens, const struct production_rule *root_rule) {
    print_with_color(TEXT_COLOR_LIGHT_RED, "Parsing with root rule %s\n", root_rule->name);
    const erule_p_harr_p_harr charts = make_charts(tokens, root_rule);
    for (size_t i = 0; i < charts.len; i++) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "Chart %zu:\n", i);
        print_chart(charts.data[i]);
    }
    printf("\n");

    return get_tree_root(charts, root_rule);
}

struct earley_rule *parse_full_file(const pp_token_harr tokens) {
    return parse(tokens, &tr_preprocessing_file);
}

pp_token_harr preprocess_full_file_tree(const struct earley_rule *const root) {
    return preprocess_tree(*root->completed_from.data[0]);
}

void test_parser(const pp_token_harr tokens) {
    const struct earley_rule *const root = parse_full_file(tokens);
    if (root == NULL) {
        printf("No tree to print; parsing failed.\n");
        return;
    }
    print_with_color(TEXT_COLOR_LIGHT_RED, "Full tree:\n");
    print_tree(root, 0);
    const pp_token_harr new_tokens = preprocess_tree(*root->completed_from.data[0]);
    print_with_color(TEXT_COLOR_LIGHT_RED, "New tokens:\n");
    print_tokens(stdout, new_tokens, true, false);
}
