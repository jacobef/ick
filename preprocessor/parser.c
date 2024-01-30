#include "preprocessor/parser.h"
#include "data_structures/vector.h"
#include "debug/color_print.h"

typedef struct earley_rule *erule_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p)

struct earley_rule {
    struct production_rule *lhs;
    struct alternative rhs;
    struct symbol *dot;
    erule_p_vec *origin_chart;
    erule_p_vec completed_from;
};

static erule_p_vec get_earley_rules(struct production_rule *rule, erule_p_vec *origin) {
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
    if (rule.dot == rule.rhs.symbols + rule.rhs.n || rule.dot->type != NON_TERMINAL) {
        erule_p_vec empty;
        erule_p_vec_init(&empty, 0);
        return empty;
    } else {
        erule_p_vec out = get_earley_rules(rule.dot->val.rule, rule_chart);
        for (size_t i = 0; i < out.n_elements; i++) {
            print_with_color(TEXT_COLOR_LIGHT_BLUE, "{predictor} ");
            print_rule(*out.arr[i]);
            print_with_color(TEXT_COLOR_LIGHT_CYAN, " {from this source:} ");
            print_rule(rule);
            printf("\n");
        }
        return out;
    }
}

static bool rule_is_duplicate(erule_p_vec rules, struct earley_rule rule) {
    for (size_t i = 0; i < rules.n_elements; i++) {
        if (rule.lhs == rules.arr[i]->lhs && rule.dot == rules.arr[i]->dot && rule.origin_chart == rules.arr[i]->origin_chart) {
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
    switch (sym.type) {
        case TERMINAL_FN:
            return sym.val.terminal_fn(token);
        case TERMINAL_STR:
            return token_is_str(token, sym.val.terminal_str);
        case NON_TERMINAL:
            return false;
    }
}

static void complete(struct earley_rule rule, erule_p_vec *out) {
    for (size_t i = 0; i < rule.origin_chart->n_elements; i++) {
        struct earley_rule possible_origin = *rule.origin_chart->arr[i];
        if (possible_origin.dot->type == NON_TERMINAL && !is_completed(possible_origin) && possible_origin.dot->val.rule == rule.lhs) {
            erule_p_vec new_completed_from;
            erule_p_vec_init(&new_completed_from, possible_origin.completed_from.n_elements + 1);
            erule_p_vec_append_all(&new_completed_from, &possible_origin.completed_from);
            struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                    .lhs=possible_origin.lhs, .rhs=possible_origin.rhs, .dot=possible_origin.dot + 1,
                    .origin_chart=possible_origin.origin_chart, .completed_from=new_completed_from
            };
            if (!rule_is_duplicate(*out, *to_append)) {
                erule_p_vec_append(out, to_append);
                print_with_color(TEXT_COLOR_LIGHT_GREEN, "{completer} ");
                print_rule(*to_append);
                print_with_color(TEXT_COLOR_LIGHT_CYAN, " {from this source:} ");
                print_rule(rule);
                printf("\n");
            }
        }
    }
}

static erule_p_vec *next_chart(erule_p_vec *old_chart, struct preprocessing_token token) {
    erule_p_vec *out = MALLOC(sizeof(erule_p_vec));
    erule_p_vec_init(out, 0);
    // Scan
    for (size_t i = 0; i < old_chart->n_elements; i++) {
        struct earley_rule rule = *old_chart->arr[i];
        if (!is_completed(rule) && check_terminal(*rule.dot, token)) {
            struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                    .lhs=rule.lhs, .rhs=rule.rhs, .dot=rule.dot+1, .origin_chart=rule.origin_chart, .completed_from=rule.completed_from
            };
            print_with_color(TEXT_COLOR_YELLOW, "{scanner} ");
            print_rule(*to_append);
            printf("\n");
            erule_p_vec_append(out, to_append);
        }
    }
    // Complete and predict in a loop
    for (size_t i = 0; i < out->n_elements; i++) {
        struct earley_rule rule = *out->arr[i];
        if (is_completed(rule)) {
            complete(rule, out);
        }
        recursively_predict(rule, out);
    }

    return out;
}

static void print_symbol(struct symbol sym) {
    switch (sym.type) {
        case NON_TERMINAL:
            printf("%s ", sym.val.rule->name);
            break;
        case TERMINAL_FN:
            printf("[function] ");
            break;
        case TERMINAL_STR:
            if (strcmp("\n", (const char*)sym.val.terminal_str) == 0) {
                print_with_color(TEXT_COLOR_GREEN, "[newline] ");
            } else {
                print_with_color(TEXT_COLOR_GREEN, "'%s' ", sym.val.terminal_str);
            }
            break;
    }
}

static void print_rule(struct earley_rule rule) {
    printf("%s -> ", rule.lhs->name);
    for (size_t i = 0; i < rule.rhs.n; i++) {
        if (rule.dot == &rule.rhs.symbols[i]) {
            print_with_color(TEXT_COLOR_LIGHT_RED, "O ");
        }
        print_symbol(rule.rhs.symbols[i]);
    }
    if (rule.dot == rule.rhs.symbols + rule.rhs.n) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "O ");
    }
}

static void print_chart(erule_p_vec *chart) {
    for (size_t i = 0; i < chart->n_elements; i++) {
        struct earley_rule rule = *chart->arr[i];
        print_rule(rule);
        printf("\n");
    }
}

static void print_token(struct preprocessing_token token) {
    for (const unsigned char *it = token.first; it != token.last+1; it++) {
        switch (*it) {
            case ' ':
                if (token.last - token.first == 0) printf("[space]");
                else printf(" ");
                break;
            case '\t':
                printf("[tab]");
                break;
            case '\n':
                printf("[newline]");
                break;
            default:
                printf("%c", *it);
        }
    }
}

void test_next_chart(pp_token_vec tokens) {
     erule_p_vec initial_chart;
     erule_p_vec_init(&initial_chart, 1);
     erule_p_vec empty;
     erule_p_vec_init(&empty, 0);

     struct earley_rule *S_rule = MALLOC(sizeof(struct earley_rule));

     *S_rule = (struct earley_rule){
         .lhs=&preprocessing_file, .rhs=preprocessing_file.alternatives[0], .dot=preprocessing_file.alternatives[0].symbols, .origin_chart=&initial_chart, .completed_from=empty
     };
     erule_p_vec_append(&initial_chart, S_rule);

     print_with_color(TEXT_COLOR_LIGHT_RED, "\nInitial Chart:\n");
     print_chart(&initial_chart);

    for (size_t i = 0; i < initial_chart.n_elements; i++) {
        struct earley_rule rule = *initial_chart.arr[i];
        if (is_completed(rule)) {
            complete(rule, &initial_chart);
        }
        recursively_predict(rule, &initial_chart);
    }
//     printf("Amended Initial Chart:\n");
//     print_chart(&initial_chart);

     erule_p_vec *old_chart = &initial_chart;
     erule_p_vec *new_chart = &initial_chart;
     for (size_t i = 0; i < tokens.n_elements; i++) {
         struct preprocessing_token token = tokens.arr[i];
         print_with_color(TEXT_COLOR_RED, "\nChart after processing token %zu (", i);
         set_color(TEXT_COLOR_MAGENTA);
         print_token(token);
         clear_color();
         print_with_color(TEXT_COLOR_RED, "):\n");
         new_chart = next_chart(old_chart, tokens.arr[i]);
         old_chart = new_chart;
//         print_chart(new_chart);
     }
}
