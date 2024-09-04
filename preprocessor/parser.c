#include "preprocessor/parser.h"
#include "preprocessor/diagnostics.h"
#include "preprocessor/macro_expansion.h"
#include "preprocessor/conditional_inclusion.h"
#include "data_structures/vector.h"
#include "debug/color_print.h"

static erule_p_vec get_earley_rules(const struct production_rule *const rule, erule_p_vec *const origin) {
    erule_p_vec out = erule_p_vec_new(rule->n);
    for (size_t i = 0; i < rule->n; i ++) {
        struct earley_rule *const to_append = MALLOC(sizeof(struct earley_rule));
        *to_append = (struct earley_rule) {
                .lhs=rule, .rhs=rule->alternatives[i], .dot=rule->alternatives[i].symbols, .origin_chart=origin, .completed_from=erule_p_vec_new(0)
        };
        erule_p_vec_append(&out, to_append);
    }
    return out;
}

static void print_rule(struct earley_rule rule);

static erule_p_vec predict(const struct earley_rule rule, erule_p_vec *const rule_chart) {
    if (rule.dot == rule.rhs.symbols + rule.rhs.n || rule.dot->is_terminal) {
        return erule_p_vec_new(0);
    } else {
        const erule_p_vec out = get_earley_rules(rule.dot->val.rule, rule_chart);
        for (size_t i = 0; i < out.arr.len; i++) {
            print_with_color(TEXT_COLOR_LIGHT_BLUE, "{predictor} ");
            print_rule(*out.arr.data[i]);
            print_with_color(TEXT_COLOR_LIGHT_CYAN, " {source:} ");
            print_rule(rule);
            printf("\n");
        }
        return out;
    }
}

static bool rule_is_duplicate(const erule_p_vec rules, const struct earley_rule rule) {
    for (size_t i = 0; i < rules.arr.len; i++) {
        if (rule.lhs == rules.arr.data[i]->lhs // Left hand rule is the same
        && rule.rhs.tag == rules.arr.data[i]->rhs.tag  // Alternative is the same
        && rule.dot - rule.rhs.symbols == rules.arr.data[i]->dot - rules.arr.data[i]->rhs.symbols // Dot is in the same place
        && rule.origin_chart == rules.arr.data[i]->origin_chart // Same origin
        ) {
            return true;
        }
    }
    return false;
}

static void recursively_predict(const struct earley_rule rule, erule_p_vec *const rule_chart) {
    const erule_p_vec first_predictions = predict(rule, rule_chart);
    for (size_t i = 0; i < first_predictions.arr.len; i++) {
        struct earley_rule *const predict_from = MALLOC(sizeof(struct earley_rule));
        *predict_from = *first_predictions.arr.data[i];
        if (!rule_is_duplicate(*rule_chart, *predict_from)) {
            erule_p_vec_append(rule_chart, predict_from);
            recursively_predict(*predict_from, rule_chart);
        }
    }
}

static bool is_completed(const struct earley_rule rule) {
    return rule.dot == rule.rhs.symbols + rule.rhs.n;
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
    for (size_t i = 0; i < rule->origin_chart->arr.len; i++) {
        const struct earley_rule possible_origin = *rule->origin_chart->arr.data[i];
        if (!possible_origin.dot->is_terminal && !is_completed(possible_origin) && possible_origin.dot->val.rule == rule->lhs) {
            erule_p_vec new_completed_from = erule_p_vec_new(possible_origin.completed_from.arr.len + 1);
            erule_p_vec_append_all(&new_completed_from, possible_origin.completed_from);
            erule_p_vec_append(&new_completed_from, rule);
            struct earley_rule *const to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                    .lhs=possible_origin.lhs, .rhs=possible_origin.rhs, .dot=possible_origin.dot + 1,
                    .origin_chart=possible_origin.origin_chart, .completed_from=new_completed_from
            };
            if (!rule_is_duplicate(*out, *to_append)) {
                erule_p_vec_append(out, to_append);
                print_with_color(TEXT_COLOR_LIGHT_GREEN, "{completer} ");
                print_rule(*to_append);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{trigger:} ");
                print_rule(*rule);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{origin:} ");
                print_rule(possible_origin);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, "\n\t{completed from:} ");
                for (size_t j = 0; j < to_append->completed_from.arr.len; j++) {
                    printf("\n\t\t");
                    print_rule(*to_append->completed_from.arr.data[j]);
                }
                printf("\n");
            }
        }
    }
}

static void scan(const struct earley_rule rule, const struct preprocessing_token token, erule_p_vec *const out) {
    if (is_completed(rule) || !rule.dot->is_terminal || !check_terminal(*rule.dot, token)) {
        return;
    }
    // Shallow copy the old rule into the new one
    struct earley_rule *const scanned_rule = MALLOC(sizeof(struct earley_rule));
    memcpy(scanned_rule, &rule, sizeof(struct earley_rule));
    // Deep copy the symbols over to the new rule
    scanned_rule->rhs.symbols = MALLOC(sizeof(struct symbol) * rule.rhs.n);
    memcpy(scanned_rule->rhs.symbols, rule.rhs.symbols, sizeof(struct symbol) * rule.rhs.n);
    // Make the dot point to the new rule's symbols, in the same position as the old one, then advance it by 1
    scanned_rule->dot = scanned_rule->rhs.symbols + (rule.dot - rule.rhs.symbols) + 1;
    // Put the token in the terminal that was scanned over
    (scanned_rule->dot - 1)->val.terminal.token = token;
    // Mark that terminal as containing a token
    (scanned_rule->dot - 1)->val.terminal.is_filled = true;

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
                    print_with_color(TEXT_COLOR_GREEN, "%s ", sym.val.terminal.matcher.str);
                }
                break;
        }
    } else {
        printf("%s ", sym.val.rule->name);
    }
}

static void print_rule(const struct earley_rule rule) {
    printf("%s -> ", rule.lhs->name);
    for (size_t i = 0; i < rule.rhs.n; i++) {
        if (rule.dot == &rule.rhs.symbols[i]) {
            print_with_color(TEXT_COLOR_LIGHT_PURPLE, "• ");
        }
        print_symbol(rule.rhs.symbols[i]);
    }
    if (rule.dot == rule.rhs.symbols + rule.rhs.n) {
        print_with_color(TEXT_COLOR_LIGHT_PURPLE, "• ");
    }
}

void print_chart(const erule_p_vec *const chart) {
    for (size_t i = 0; i < chart->arr.len; i++) {
        const struct earley_rule rule = *chart->arr.data[i];
        print_rule(rule);
        printf("\n");
    }
}

static void print_token(const struct preprocessing_token token) {
    for (size_t j = 0; j < token.name.n; j++) {
        if (token.name.chars[j] == '\n') {
            printf("[newline]");
        } else {
            printf("%c", token.name.chars[j]);
        }
    }
}

erule_p_vec_p_vec make_charts(const pp_token_vec tokens, const struct production_rule *const start_rule) {
    erule_p_vec_p_vec out = erule_p_vec_p_vec_new(0);

    erule_p_vec *const initial_chart = MALLOC(sizeof(struct erule_p_vec));
    *initial_chart = erule_p_vec_new(1);
    erule_p_vec_p_vec_append(&out, initial_chart);

    for (size_t i = 0; i < start_rule->n; i++) {
        struct earley_rule *const rule = MALLOC(sizeof(struct earley_rule));
        *rule = (struct earley_rule) {
            .lhs=start_rule, .rhs=start_rule->alternatives[i], .dot=start_rule->alternatives[i].symbols, .origin_chart=initial_chart, .completed_from=erule_p_vec_new(0)
        };
        erule_p_vec_append(initial_chart, rule);
    }

    print_with_color(TEXT_COLOR_LIGHT_RED, "\nInitial Chart:\n");
    print_chart(initial_chart);

    for (size_t i = 0; i < initial_chart->arr.len; i++) {
        struct earley_rule *const rule = initial_chart->arr.data[i];
        complete(rule, initial_chart);
        recursively_predict(*rule, initial_chart);
    }

    const erule_p_vec *old_chart = initial_chart;
    for (size_t i = 0; i < tokens.arr.len; i++) {
        const struct preprocessing_token token = tokens.arr.data[i];
        print_with_color(TEXT_COLOR_RED, "\nChart after processing token %zu (", i);
        set_color(TEXT_COLOR_GREEN);
        print_token(token);
        clear_color();
        print_with_color(TEXT_COLOR_RED, "):\n");
        erule_p_vec *const new_chart = next_chart(old_chart, tokens.arr.data[i]);
        erule_p_vec_p_vec_append(&out, new_chart);
        old_chart = new_chart;
    }
    printf("\n");

    return out;
}

static void flatten_list_rules(struct earley_rule *root);
static struct earley_rule *get_tree_root(const erule_p_vec_p_vec charts, const struct production_rule *const root_rule) {
    const erule_p_vec *const final_chart = charts.arr.data[charts.arr.len-1];
    for (size_t i = 0; i < final_chart->arr.len; i++) {
        struct earley_rule *const rule = final_chart->arr.data[i];
        if (rule->lhs == root_rule && is_completed(*rule)) {
            flatten_list_rules(rule);
            return rule;
        }
    }
    return NULL;
}

static erule_p_vec flatten_list_rule(const struct earley_rule list_rule) {
    // would be cleaner if recursive, but would stack overflow for long lists

    erule_p_vec out_reversed = erule_p_vec_new(0);
    struct earley_rule current_rule = list_rule;
    while (current_rule.rhs.tag == LIST_RULE_MULTI) {
        erule_p_vec_append(&out_reversed, current_rule.completed_from.arr.data[1]);
        current_rule = *current_rule.completed_from.arr.data[0];
    }
    erule_p_vec_append(&out_reversed, current_rule.completed_from.arr.data[0]);

    erule_p_vec out = erule_p_vec_new(0);
    for (ssize_t i = (ssize_t)out_reversed.arr.len - 1; i >= 0; i--) {
        erule_p_vec_append(&out, out_reversed.arr.data[i]);
    }
    erule_p_vec_free_internals(&out_reversed);
    return out;
}

static void flatten_list_rules(struct earley_rule *const root) {
    if (root->lhs->is_list_rule) { // TODO autodetect list rules
        const erule_p_vec old_completed_from = root->completed_from;
        root->completed_from = flatten_list_rule(*root);
        erule_p_vec_free_internals((erule_p_vec*)&old_completed_from);
    }
    for (size_t i = 0; i < root->completed_from.arr.len; i++) {
        flatten_list_rules(root->completed_from.arr.data[i]);
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
    for (size_t i = 0; i < root->completed_from.arr.len; i++) {
        print_tree(root->completed_from.arr.data[i], indent + 1);
    }
}

static pp_token_vec get_text_line_tokens(const struct earley_rule text_line_rule) {
    pp_token_vec text_line_tokens = pp_token_vec_new(0);

    const struct earley_rule *const tokens_not_starting_with_hashtag_opt_rule = text_line_rule.completed_from.arr.data[0];
    if (tokens_not_starting_with_hashtag_opt_rule->rhs.tag == OPT_NONE) {
        return text_line_tokens;
    }
    const struct earley_rule *const tokens_not_starting_with_hashtag_rule = tokens_not_starting_with_hashtag_opt_rule->completed_from.arr.data[0];

    for (size_t i = 0; i < tokens_not_starting_with_hashtag_rule->completed_from.arr.len; i++) {
        const struct earley_rule *const non_hashtag_rule = tokens_not_starting_with_hashtag_rule->completed_from.arr.data[i];
        pp_token_vec_append(&text_line_tokens, non_hashtag_rule->rhs.symbols[0].val.terminal.token);
    }

    return text_line_tokens;
}

static void deal_with_macros(const struct earley_rule root) {
    const struct earley_rule group_opt_rule = *root.completed_from.arr.data[0];
    if (group_opt_rule.rhs.tag == OPT_NONE) {
        printf("no groups\n");
        return;
    }
    const struct earley_rule group_rule = *group_opt_rule.completed_from.arr.data[0];

    str_view_macro_args_and_body_map macros = str_view_macro_args_and_body_map_new(0);

    pp_token_vec text_section = pp_token_vec_new(0);
    for (size_t i = 0; i < group_rule.completed_from.arr.len; i++) {
        const struct earley_rule group_part_rule = *group_rule.completed_from.arr.data[i];
        switch ((enum group_part_tag)group_part_rule.rhs.tag) {
            case GROUP_PART_CONTROL: {
                const struct earley_rule control_line_rule = *group_part_rule.completed_from.arr.data[0];
                switch((enum control_line_tag)control_line_rule.rhs.tag) {
                    case CONTROL_LINE_DEFINE_OBJECT_LIKE: {
                        define_object_like_macro(control_line_rule, &macros);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(&macros);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS:
                    case CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS: {
                        define_function_like_macro(control_line_rule, &macros);
                        print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                        print_macros(&macros);
                        printf("\n");
                        break;
                    }
                    case CONTROL_LINE_UNDEF: {
                        // Removes the macro if it exists; does nothing if it doesn't
                        str_view_macro_args_and_body_map_remove(&macros, control_line_rule.completed_from.arr.data[0]->rhs.symbols[0].val.terminal.token.name);
                        break;
                    }
                }
                break;
            }
            case GROUP_PART_TEXT: {
                const struct earley_rule text_line_rule = *group_part_rule.completed_from.arr.data[0];
                const pp_token_vec text_line_tokens = replace_macros(get_text_line_tokens(text_line_rule), macros);
                pp_token_vec_append_all(&text_section, text_line_tokens);
                break;
            }
            case GROUP_PART_IF: {
                const struct earley_rule if_section_rule = *group_part_rule.completed_from.arr.data[0];
                struct earley_rule *const included_group = eval_if_section(if_section_rule);
                print_with_color(TEXT_COLOR_LIGHT_RED, "Tree of included if section:\n");
                print_tree(included_group, 0);
                break;
            }
        }
    }
    print_tokens(text_section, true);
}

void test_parser(const pp_token_vec tokens) {
    const erule_p_vec_p_vec charts = make_charts(tokens, &tr_preprocessing_file);
    for (size_t i = 0; i < charts.arr.len; i++) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "Chart %zu:\n", i);
        print_chart(charts.arr.data[i]);
    }
    printf("\n");
    struct earley_rule *const root = get_tree_root(charts, &tr_preprocessing_file);
    if (root == NULL) {
        printf("No tree to print; parsing failed.\n");
        return;
    }
    print_with_color(TEXT_COLOR_LIGHT_RED, "Full tree:\n");
    print_tree(root, 0);
    deal_with_macros(*root);
    // print_with_color(TEXT_COLOR_LIGHT_RED, "Full tree:\n");
    // print_tree(root, 0);
}
