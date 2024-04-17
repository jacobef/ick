#include "preprocessor/char_parser.h"
#include "preprocessor/diagnostics.h"
#include "data_structures/vector.h"
#include "debug/color_print.h"
#include "macro_expansion.h"


static char_erule_p_vec get_earley_rules(const struct char_production_rule *rule, char_erule_p_vec *origin) {
    char_erule_p_vec out;
    char_erule_p_vec_init(&out, rule->n);
    for (size_t i = 0; i < rule->n; i++) {
        char_erule_p_vec empty;
        char_erule_p_vec_init(&empty, 0);
        struct char_earley_rule *to_append = MALLOC(sizeof(struct char_earley_rule));
        *to_append = (struct char_earley_rule) {
                .lhs=rule, .rhs=rule->alternatives[i], .dot=rule->alternatives[i].symbols, .origin_chart=origin, .completed_from=empty
        };
        char_erule_p_vec_append(&out, to_append);
    }
    return out;
}

static void print_rule(struct char_earley_rule rule);

static char_erule_p_vec predict(struct char_earley_rule rule, char_erule_p_vec *rule_chart) {
    if (rule.dot == rule.rhs.symbols + rule.rhs.n || rule.dot->is_terminal) {
        char_erule_p_vec empty;
        char_erule_p_vec_init(&empty, 0);
        return empty;
    } else {
        char_erule_p_vec out = get_earley_rules(rule.dot->val.rule, rule_chart);
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

static bool rule_is_duplicate(char_erule_p_vec rules, struct char_earley_rule rule) {
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

static void recursively_predict(struct char_earley_rule rule, char_erule_p_vec *rule_chart) {
    char_erule_p_vec first_predictions = predict(rule, rule_chart);
    for (size_t i = 0; i < first_predictions.n_elements; i++) {
        struct char_earley_rule *predict_from = MALLOC(sizeof(struct char_earley_rule));
        *predict_from = *first_predictions.arr[i];
        if (!rule_is_duplicate(*rule_chart, *predict_from)) {
            char_erule_p_vec_append(rule_chart, predict_from);
            recursively_predict(*predict_from, rule_chart);
        }
    }
}

static bool is_completed(struct char_earley_rule rule) {
    return rule.dot == rule.rhs.symbols + rule.rhs.n;
}

static bool check_terminal(struct char_symbol sym, unsigned char token) {
    if (sym.is_terminal) {
        switch(sym.val.terminal.type) {
            case CHAR_TERMINAL_FN:
                return sym.val.terminal.matcher.fn(token);
            case CHAR_TERMINAL_CHAR:
                return token == sym.val.terminal.matcher.chr;
        }
    } else {
        return false;
    }
}

static void complete(struct char_earley_rule *rule, char_erule_p_vec *out) {
    if (!is_completed(*rule)) {
        return;
    }
    for (size_t i = 0; i < rule->origin_chart->n_elements; i++) {
        struct char_earley_rule possible_origin = *rule->origin_chart->arr[i];
        if (!possible_origin.dot->is_terminal && !is_completed(possible_origin) && possible_origin.dot->val.rule == rule->lhs) {
            char_erule_p_vec new_completed_from;
            char_erule_p_vec_init(&new_completed_from, possible_origin.completed_from.n_elements + 1);
            char_erule_p_vec_append_all(&new_completed_from, &possible_origin.completed_from);
            char_erule_p_vec_append(&new_completed_from, rule);
            struct char_earley_rule *to_append = MALLOC(sizeof(struct char_earley_rule));
            *to_append = (struct char_earley_rule) {
                    .lhs=possible_origin.lhs, .rhs=possible_origin.rhs, .dot=possible_origin.dot + 1,
                    .origin_chart=possible_origin.origin_chart, .completed_from=new_completed_from
            };
            if (!rule_is_duplicate(*out, *to_append)) {
                char_erule_p_vec_append(out, to_append);
                print_with_color(TEXT_COLOR_LIGHT_GREEN, "{completer} ");
                print_rule(*to_append);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, " {source:} ");
                print_rule(*rule);
                printf("\n");
            }
        }
    }
}

static void scan(struct char_earley_rule rule, unsigned char token, char_erule_p_vec *out) {
    if (is_completed(rule) || !rule.dot->is_terminal || !check_terminal(*rule.dot, token)) {
        return;
    }
    // Shallow copy the old rule into the new one
    struct char_earley_rule *scanned_rule = MALLOC(sizeof(struct char_earley_rule));
    memcpy(scanned_rule, &rule, sizeof(struct char_earley_rule));
    // Deep copy the symbols over to the new rule
    scanned_rule->rhs.symbols = MALLOC(sizeof(struct char_symbol) * rule.rhs.n);
    memcpy(scanned_rule->rhs.symbols, rule.rhs.symbols, sizeof(struct char_symbol) * rule.rhs.n);
    // Make the dot point to the new rule's symbols, in the same position as the old one, then advance it by 1
    scanned_rule->dot = scanned_rule->rhs.symbols + (rule.dot - rule.rhs.symbols) + 1;
    // Put the token in the terminal that was scanned over
    (scanned_rule->dot - 1)->val.terminal.token = token;
    // Mark that terminal as containing a token
    (scanned_rule->dot - 1)->val.terminal.is_filled = true;

    print_with_color(TEXT_COLOR_YELLOW, "{scanner} ");
    print_rule(*scanned_rule);
    printf("\n");

    char_erule_p_vec_append(out, scanned_rule);
}

static char_erule_p_vec *next_chart(char_erule_p_vec *old_chart, unsigned char token) {
    char_erule_p_vec *out = MALLOC(sizeof(char_erule_p_vec));
    char_erule_p_vec_init(out, 0);
    // Scan
    for (size_t i = 0; i < old_chart->n_elements; i++) {
        scan(*old_chart->arr[i], token, out);
    }
    // Complete and predict in a loop
    for (size_t i = 0; i < out->n_elements; i++) {
        struct char_earley_rule *rule = out->arr[i];
        complete(rule, out);
        recursively_predict(*rule, out);
    }

    return out;
}

static void char_print_token(unsigned char token);

static void print_symbol(struct char_symbol sym) {
    if (sym.is_terminal) {
        switch (sym.val.terminal.type) {
            case CHAR_TERMINAL_FN:
                printf("[function] ");
                break;
            case CHAR_TERMINAL_CHAR:
                if (sym.val.terminal.matcher.chr == '\n') {
                    print_with_color(TEXT_COLOR_GREEN, "[newline] ");
                } else {
                    print_with_color(TEXT_COLOR_GREEN, "%c ", sym.val.terminal.matcher.chr);
                }
                break;
        }
        if (sym.val.terminal.is_filled) {
            printf("(filled: ");
            set_color(TEXT_COLOR_GREEN);
            char_print_token(sym.val.terminal.token);
            clear_color();
            printf(") ");
        }
    } else {
        printf("%s ", sym.val.rule->name);
    }
}

static void print_rule(struct char_earley_rule rule) {
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

void char_print_chart(char_erule_p_vec *chart) {
    for (size_t i = 0; i < chart->n_elements; i++) {
        struct char_earley_rule rule = *chart->arr[i];
        print_rule(rule);
        printf("\n");
    }
}

static void char_print_token(unsigned char token) {
    if (token == '\n') printf("[newline]");
    else printf("%c", token);
}

char_erule_p_vec_p_vec char_make_charts(uchar_vec tokens, const struct char_production_rule *root_rule) {
    char_erule_p_vec_p_vec out;
    char_erule_p_vec_p_vec_init(&out, 0);

    char_erule_p_vec *initial_chart = MALLOC(sizeof(struct char_erule_p_vec));
    char_erule_p_vec_init(initial_chart, 1);
    char_erule_p_vec_p_vec_append(&out, initial_chart);

    for (size_t i = 0; i < root_rule->n; i++) {
        char_erule_p_vec empty;
        char_erule_p_vec_init(&empty, 0);
        struct char_earley_rule *to_append = MALLOC(sizeof(struct char_earley_rule));
        *to_append = (struct char_earley_rule) {
                .lhs = root_rule, .rhs = root_rule->alternatives[i], .dot = root_rule->alternatives[i].symbols, .origin_chart = initial_chart, .completed_from = empty
        };
        char_erule_p_vec_append(initial_chart, to_append);
    }

    print_with_color(TEXT_COLOR_LIGHT_RED, "\nInitial Chart:\n");
    char_print_chart(initial_chart);

    for (size_t i = 0; i < initial_chart->n_elements; i++) {
        struct char_earley_rule *rule = initial_chart->arr[i];
        complete(rule, initial_chart);
        recursively_predict(*rule, initial_chart);
    }
    //     printf("Amended Initial Chart:\n");
    //     print_chart(&initial_chart);

    char_erule_p_vec *old_chart = initial_chart;
    char_erule_p_vec *new_chart;
    for (size_t i = 0; i < tokens.n_elements; i++) {
        unsigned char token = tokens.arr[i];
        print_with_color(TEXT_COLOR_RED, "\nChart after processing token %zu (", i);
        set_color(TEXT_COLOR_GREEN);
        char_print_token(token);
        clear_color();
        print_with_color(TEXT_COLOR_RED, "):\n");
        new_chart = next_chart(old_chart, tokens.arr[i]);
        char_erule_p_vec_p_vec_append(&out, new_chart);
        old_chart = new_chart;
        //         print_chart(new_chart);
    }
    printf("\n");

    return out;
}

static void char_flatten_list_rules(struct char_earley_rule *rule);
static struct char_earley_rule *char_get_tree_root(char_erule_p_vec final_chart, const struct char_production_rule *expected_root) {
    for (size_t i = 0; i < final_chart.n_elements; i++) {
        struct char_earley_rule *rule = final_chart.arr[i];
        if (rule->lhs == expected_root && is_completed(*rule)) {
            char_flatten_list_rules(rule);
            return rule;
        }
    }
    return NULL;
}

static char_erule_p_vec char_flatten_list_rule(struct char_earley_rule list_rule) {
    // would be cleaner if recursive, but would stack overflow for long lists

    char_erule_p_vec out_reversed;
    char_erule_p_vec_init(&out_reversed, 0);
    struct char_earley_rule current_rule = list_rule;
    while (current_rule.rhs.tag == CR_LIST_RULE_MULTI) {
        char_erule_p_vec_append(&out_reversed, current_rule.completed_from.arr[1]);
        current_rule = *current_rule.completed_from.arr[0];
    }
    char_erule_p_vec_append(&out_reversed, current_rule.completed_from.arr[0]);

    char_erule_p_vec out;
    char_erule_p_vec_init(&out, 0);
    for (ssize_t i = (ssize_t)out_reversed.n_elements - 1; i >= 0; i--) {
        char_erule_p_vec_append(&out, out_reversed.arr[i]);
    }
    char_erule_p_vec_free_internals(&out_reversed);
    return out;
}

static bool char_is_list_rule(const struct char_production_rule *rule) {
    if (rule->n != 2) return false;
    bool found_lr_symbol = false;
    bool found_concrete_symbol = false;
    for (size_t i = 0; i < rule->alternatives[1].n; i++) {
        struct char_symbol symbol = rule->alternatives[1].symbols[i];
        if (!symbol.is_terminal && !found_lr_symbol && symbol.val.rule == rule) {
            found_lr_symbol = true;
        } else if (!symbol.is_terminal) {
            found_concrete_symbol = true;
        }
    }
    return found_lr_symbol && found_concrete_symbol;
}

static void char_flatten_list_rules(struct char_earley_rule *root) {
    if (char_is_list_rule(root->lhs)) {
        char_erule_p_vec old_completed_from = root->completed_from;
        root->completed_from = char_flatten_list_rule(*root);
        char_erule_p_vec_free_internals(&old_completed_from);
    }
    for (size_t i = 0; i < root->completed_from.n_elements; i++) {
        char_flatten_list_rules(root->completed_from.arr[i]);
    }
}

void char_print_tree(struct char_earley_rule *root, size_t indent) {
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
        char_print_tree(root->completed_from.arr[i], indent+1);
    }
}

struct char_earley_rule *char_parse(uchar_vec tokens, const struct char_production_rule *root_rule) {
    char_erule_p_vec_p_vec charts = char_make_charts(tokens, root_rule);
    struct char_earley_rule *root = char_get_tree_root(*charts.arr[charts.n_elements-1], &cr_character_constant);
    return root;
}

void char_test_parser(uchar_vec tokens) {
    char_erule_p_vec_p_vec charts = char_make_charts(tokens, &cr_character_constant);
    for (size_t i = 0; i < charts.n_elements; i++) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "Chart %zu:\n", i);
        char_print_chart(charts.arr[i]);
    }
    printf("\n");
    struct char_earley_rule *root = char_get_tree_root(*charts.arr[charts.n_elements-1], &cr_character_constant);
    if (root == NULL) {
        printf("No tree to print; parsing failed.\n");
        return;
    }
    print_with_color(TEXT_COLOR_LIGHT_RED, "Full tree:\n");
    char_print_tree(root, 0);
}
