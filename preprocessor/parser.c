#include "preprocessor/parser.h"
#include "preprocessor/diagnostics.h"
#include "data_structures/vector.h"
#include "debug/color_print.h"
#include "macro_expansion.h"


static erule_p_vec get_earley_rules(const struct production_rule *rule, erule_p_vec *origin) {
    erule_p_vec out;
    erule_p_vec_init(&out, rule->n);
    for (size_t i = 0; i < rule->n; i++) {
        erule_p_vec empty;
        erule_p_vec_init(&empty, 0);
        struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
        *to_append = (struct earley_rule) {
                .lhs=rule, .rhs=rule->alternatives[i], .dot=rule->alternatives[i].symbols, .origin_chart=origin, .completed_from=empty
        };
        erule_p_vec_append(&out, to_append);
    }
    return out;
}

static void print_rule(struct earley_rule rule);

static erule_p_vec predict(struct earley_rule rule, erule_p_vec *rule_chart) {
    if (rule.dot == rule.rhs.symbols + rule.rhs.n || rule.dot->is_terminal) {
        erule_p_vec empty;
        erule_p_vec_init(&empty, 0);
        return empty;
    } else {
        erule_p_vec out = get_earley_rules(rule.dot->val.rule, rule_chart);
        for (size_t i = 0; i < out.n_elements; i++) {
            print_with_color(TEXT_COLOR_LIGHT_BLUE, "{predictor} ");
            print_rule(*out.arr[i]);
            print_with_color(TEXT_COLOR_LIGHT_CYAN, " {source:} ");
            print_rule(rule);
            printf("\n");
        }
        return out;
    }
}

static bool rule_is_duplicate(erule_p_vec rules, struct earley_rule rule) {
    for (size_t i = 0; i < rules.n_elements; i++) {
        if (rule.lhs == rules.arr[i]->lhs // Left hand rule is the same
        && rule.rhs.tag == rules.arr[i]->rhs.tag  // Alternative is the same
        && rule.dot - rule.rhs.symbols == rules.arr[i]->dot - rules.arr[i]->rhs.symbols // Dot is in the same place
        && rule.origin_chart == rules.arr[i]->origin_chart // Same origin
        ) {
            return true;
        }
    }
    return false;
}

static void recursively_predict(struct earley_rule rule, erule_p_vec *rule_chart) {
    erule_p_vec first_predictions = predict(rule, rule_chart);
    for (size_t i = 0; i < first_predictions.n_elements; i++) {
        struct earley_rule *predict_from = MALLOC(sizeof(struct earley_rule));
        *predict_from = *first_predictions.arr[i];
        if (!rule_is_duplicate(*rule_chart, *predict_from)) {
            erule_p_vec_append(rule_chart, predict_from);
            recursively_predict(*predict_from, rule_chart);
        }
    }
}

static bool is_completed(struct earley_rule rule) {
    return rule.dot == rule.rhs.symbols + rule.rhs.n;
}

static bool check_terminal(struct symbol sym, struct preprocessing_token token) {
    if (sym.is_terminal) {
        switch(sym.val.terminal.type) {
            case TERMINAL_FN:
                return sym.val.terminal.matcher.fn(token);
            case TERMINAL_STR:
                return token_is_str(token, sym.val.terminal.matcher.str);
        }
    } else {
        return false;
    }
}

static void complete(struct earley_rule *rule, erule_p_vec *out) {
    if (!is_completed(*rule)) {
        return;
    }
    for (size_t i = 0; i < rule->origin_chart->n_elements; i++) {
        struct earley_rule possible_origin = *rule->origin_chart->arr[i];
        if (!possible_origin.dot->is_terminal && !is_completed(possible_origin) && possible_origin.dot->val.rule == rule->lhs) {
            erule_p_vec new_completed_from;
            erule_p_vec_init(&new_completed_from, possible_origin.completed_from.n_elements + 1);
            erule_p_vec_append_all(&new_completed_from, &possible_origin.completed_from);
            erule_p_vec_append(&new_completed_from, rule);
            struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                    .lhs=possible_origin.lhs, .rhs=possible_origin.rhs, .dot=possible_origin.dot + 1,
                    .origin_chart=possible_origin.origin_chart, .completed_from=new_completed_from
            };
            if (!rule_is_duplicate(*out, *to_append)) {
                erule_p_vec_append(out, to_append);
                print_with_color(TEXT_COLOR_LIGHT_GREEN, "{completer} ");
                print_rule(*to_append);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, " {source:} ");
                print_rule(*rule);
                printf("\n");
            }
        }
    }
}

static void scan(struct earley_rule rule, struct preprocessing_token token, erule_p_vec *out) {
    if (is_completed(rule) || !rule.dot->is_terminal || !check_terminal(*rule.dot, token)) {
        return;
    }
    // Shallow copy the old rule into the new one
    struct earley_rule *scanned_rule = MALLOC(sizeof(struct earley_rule));
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

static erule_p_vec *next_chart(erule_p_vec *old_chart, struct preprocessing_token token) {
    erule_p_vec *out = MALLOC(sizeof(erule_p_vec));
    erule_p_vec_init(out, 0);
    // Scan
    for (size_t i = 0; i < old_chart->n_elements; i++) {
        scan(*old_chart->arr[i], token, out);
    }
    // Complete and predict in a loop
    for (size_t i = 0; i < out->n_elements; i++) {
        struct earley_rule *rule = out->arr[i];
        complete(rule, out);
        recursively_predict(*rule, out);
    }

    return out;
}

static void print_token(struct preprocessing_token token);

static void print_symbol(struct symbol sym) {

    if (sym.is_terminal) {
        switch (sym.val.terminal.type) {
            case TERMINAL_FN:
                printf("[function] ");
                break;
            case TERMINAL_STR:
                if (strcmp("\n", (const char *)sym.val.terminal.matcher.str) == 0) {
                    print_with_color(TEXT_COLOR_GREEN, "[newline] ");
                } else {
                    print_with_color(TEXT_COLOR_GREEN, "'%s' ", sym.val.terminal.matcher.str);
                }
                break;
        }
        if (sym.val.terminal.is_filled) {
            printf("(filled: ");
            set_color(TEXT_COLOR_GREEN);
            print_token(sym.val.terminal.token);
            clear_color();
            printf(") ");
        }
    } else {
        printf("%s ", sym.val.rule->name);
    }
}

static void print_rule(struct earley_rule rule) {
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

void print_chart(erule_p_vec *chart) {
    for (size_t i = 0; i < chart->n_elements; i++) {
        struct earley_rule rule = *chart->arr[i];
        print_rule(rule);
        printf("\n");
    }
}

static void print_token(struct preprocessing_token token) {
    for (size_t j = 0; j < token.name.n; j++) {
        if (token.name.first[j] == '\n') {
            printf("[newline]");
        } else {
            printf("%c", token.name.first[j]);
        }
    }
}

erule_p_vec_p_vec make_charts(pp_token_vec tokens) {
    erule_p_vec_p_vec out;
    erule_p_vec_p_vec_init(&out, 0);

    erule_p_vec *initial_chart = MALLOC(sizeof(struct erule_p_vec));
    erule_p_vec_init(initial_chart, 1);
    erule_p_vec_p_vec_append(&out, initial_chart);

    erule_p_vec empty;
    erule_p_vec_init(&empty, 0);

    struct earley_rule *S_rule = MALLOC(sizeof(struct earley_rule));

    *S_rule = (struct earley_rule){
        .lhs=&preprocessing_file, .rhs=preprocessing_file.alternatives[0], .dot=preprocessing_file.alternatives[0].symbols, .origin_chart=initial_chart, .completed_from=empty
    };
    erule_p_vec_append(initial_chart, S_rule);

    print_with_color(TEXT_COLOR_LIGHT_RED, "\nInitial Chart:\n");
    print_chart(initial_chart);

    for (size_t i = 0; i < initial_chart->n_elements; i++) {
        struct earley_rule *rule = initial_chart->arr[i];
        complete(rule, initial_chart);
        recursively_predict(*rule, initial_chart);
    }
    //     printf("Amended Initial Chart:\n");
    //     print_chart(&initial_chart);

    erule_p_vec *old_chart = initial_chart;
    erule_p_vec *new_chart;
    for (size_t i = 0; i < tokens.n_elements; i++) {
        struct preprocessing_token token = tokens.arr[i];
        print_with_color(TEXT_COLOR_RED, "\nChart after processing token %zu (", i);
        set_color(TEXT_COLOR_GREEN);
        print_token(token);
        clear_color();
        print_with_color(TEXT_COLOR_RED, "):\n");
        new_chart = next_chart(old_chart, tokens.arr[i]);
        erule_p_vec_p_vec_append(&out, new_chart);
        old_chart = new_chart;
        //         print_chart(new_chart);
    }
    printf("\n");

    return out;
}

static struct earley_rule *get_tree_root(erule_p_vec final_chart) {
    for (size_t i = 0; i < final_chart.n_elements; i++) {
        struct earley_rule *rule = final_chart.arr[i];
        if (rule->lhs == &preprocessing_file && is_completed(*rule)) {
            return rule;
        }
    }
    return NULL;
}

static erule_p_vec flatten_list_rule(struct earley_rule list_rule) {
    // would be cleaner if recursive, but would stack overflow for long lists

    erule_p_vec out_reversed;
    erule_p_vec_init(&out_reversed, 0);
    struct earley_rule current_rule = list_rule;
    while (current_rule.rhs.tag == LIST_RULE_MULTI) {
        erule_p_vec_append(&out_reversed, current_rule.completed_from.arr[1]);
        current_rule = *current_rule.completed_from.arr[0];
    }
    erule_p_vec_append(&out_reversed, current_rule.completed_from.arr[0]);

    erule_p_vec out;
    erule_p_vec_init(&out, 0);
    for (ssize_t i = (ssize_t)out_reversed.n_elements - 1; i >= 0; i--) {
        erule_p_vec_append(&out, out_reversed.arr[i]);
    }
    erule_p_vec_free_internals(&out_reversed);
    return out;
}

static bool is_list_rule(const struct production_rule *rule) {
    if (rule->n != 2) return false;
    bool found_lr_symbol = false;
    bool found_concrete_symbol = false;
    for (size_t i = 0; i < rule->alternatives[1].n; i++) {
        struct symbol symbol = rule->alternatives[1].symbols[i];
        if (!symbol.is_terminal && !found_lr_symbol && symbol.val.rule == rule) {
            found_lr_symbol = true;
        } else if (!symbol.is_terminal) {
            found_concrete_symbol = true;
        }
    }
    return found_lr_symbol && found_concrete_symbol;
}

static void flatten_list_rules(struct earley_rule *root) {
    if (is_list_rule(root->lhs)) {
        erule_p_vec old_completed_from = root->completed_from;
        root->completed_from = flatten_list_rule(*root);
        erule_p_vec_free_internals(&old_completed_from);
    }
    for (size_t i = 0; i < root->completed_from.n_elements; i++) {
        flatten_list_rules(root->completed_from.arr[i]);
    }
}

void print_tree(struct earley_rule *root, size_t indent) {
    if (root == NULL) {
        printf("No tree to print.\n");
        return;
    }
    for (size_t i = 0; i < indent; i++) {
        printf("\t");
    }
    print_rule(*root);
    printf("\n");
    for (size_t i = 0; i < root->completed_from.n_elements; i++) {
        print_tree(root->completed_from.arr[i], indent+1);
    }
}

static pp_token_vec get_text_line_tokens(struct earley_rule text_line_rule) {
    pp_token_vec text_line_tokens;
    pp_token_vec_init(&text_line_tokens, 0);

    struct earley_rule *tokens_not_starting_with_hashtag_opt_rule = text_line_rule.completed_from.arr[0];
    if (tokens_not_starting_with_hashtag_opt_rule->rhs.tag == OPT_NONE) {
        return text_line_tokens;
    }
    struct earley_rule *tokens_not_starting_with_hashtag_rule = tokens_not_starting_with_hashtag_opt_rule->completed_from.arr[0];

    for (size_t i = 0; i < tokens_not_starting_with_hashtag_rule->completed_from.n_elements; i++) {
        struct earley_rule *non_hashtag_rule = tokens_not_starting_with_hashtag_rule->completed_from.arr[i];
        pp_token_vec_append(&text_line_tokens, non_hashtag_rule->rhs.symbols[0].val.terminal.token);
    }

    return text_line_tokens;
}

static void deal_with_macros(struct earley_rule root) {
    struct earley_rule group_opt_rule = *root.completed_from.arr[0];
    if (group_opt_rule.rhs.tag == OPT_NONE) {
        printf("no groups\n");
        return;
    }
    struct earley_rule group_rule = *group_opt_rule.completed_from.arr[0];

    str_view_macro_args_and_body_map macros;
    str_view_macro_args_and_body_map_init(&macros, 0);

    pp_token_vec text_section;
    pp_token_vec_init(&text_section, 0);
    for (size_t i = 0; i < group_rule.completed_from.n_elements; i++) {
        struct earley_rule group_part_rule = *group_rule.completed_from.arr[i];
        if (group_part_rule.rhs.tag == GROUP_PART_CONTROL) {
            struct earley_rule control_line_rule = *group_part_rule.completed_from.arr[0];
            if (control_line_rule.rhs.tag == CONTROL_LINE_DEFINE_OBJECT_LIKE) {
                define_object_like_macro(control_line_rule, &macros);
                print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                print_macros(&macros);
                printf("\n");
            } else if (control_line_rule.rhs.tag == CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS || control_line_rule.rhs.tag == CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS || control_line_rule.rhs.tag == CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS) {
                define_function_like_macro(control_line_rule, &macros);
                print_with_color(TEXT_COLOR_LIGHT_RED, "Defined macro, all macros:\n");
                print_macros(&macros);
                printf("\n");
            } else if (control_line_rule.rhs.tag == CONTROL_LINE_UNDEF) {
                if (!str_view_macro_args_and_body_map_remove(&macros, control_line_rule.completed_from.arr[0]->rhs.symbols[0].val.terminal.token.name)) {
                    preprocessor_fatal_error(0, 0, 0, "Can't undefine a macro that doesn't exist");
                }
            }
        } else if (group_part_rule.rhs.tag == GROUP_PART_TEXT) {
            struct earley_rule text_line_rule = *group_part_rule.completed_from.arr[0];
            pp_token_vec text_line_tokens = get_text_line_tokens(text_line_rule);
            pp_token_vec_append_all(&text_section, &text_line_tokens);
        }
    }
    pp_token_vec replaced = replace_macros(text_section, macros);
    print_tokens(replaced);
}

void test_parser(pp_token_vec tokens) {
    erule_p_vec_p_vec charts = make_charts(tokens);
    for (size_t i = 0; i < charts.n_elements; i++) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "Chart %zu:\n", i);
        print_chart(charts.arr[i]);
    }
    printf("\n");
    struct earley_rule *root = get_tree_root(*charts.arr[charts.n_elements-1]);
    if (root == NULL) {
        printf("No tree to print; parsing failed.\n");
        return;
    }
    flatten_list_rules(root);
    deal_with_macros(*root);
//    print_with_color(TEXT_COLOR_LIGHT_RED, "Full tree:\n");
//    print_tree(root, 0);
}
