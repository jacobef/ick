#include "preprocessor/parser.h"
#include "data_structures/vector.h"

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

static erule_p_vec predict(struct earley_rule rule, erule_p_vec *rule_chart) {
    if (rule.dot == rule.rhs.symbols+rule.rhs.n || rule.dot->is_terminal) {
        erule_p_vec empty;
        erule_p_vec_init(&empty, 0);
        return empty;
    }
    else return get_earley_rules(rule.dot->val.rule, rule_chart);
}

static void recursively_predict(struct earley_rule rule, erule_p_vec *rule_chart, erule_p_vec *out) {
    erule_p_vec first_predictions = predict(rule, rule_chart);
    for (size_t i=0; i < first_predictions.n_elements; i++) {
        struct earley_rule *predict_from = MALLOC(sizeof(struct earley_rule));
        *predict_from = *first_predictions.arr[i];
        bool is_duplicate = false;
        for (size_t j=0; j < out->n_elements; j++) {
            struct earley_rule possible_duplicate = *out->arr[j];
            if (possible_duplicate.lhs == predict_from->lhs && possible_duplicate.dot == predict_from->dot) {
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) {
            erule_p_vec_append(out, predict_from);
            recursively_predict(*predict_from, out, out);
        }
    }
}

static bool is_completed(struct earley_rule rule) {
    return rule.dot == rule.rhs.symbols + rule.rhs.n;
}

static erule_p_vec *next_chart(erule_p_vec *old_chart, struct preprocessing_token token) {
    erule_p_vec *out = MALLOC(sizeof(erule_p_vec));
    erule_p_vec_init(out, 0);
    // Scan
    for (size_t i=0; i < old_chart->n_elements; i++) {
        struct earley_rule rule = *old_chart->arr[i];
        if (!is_completed(rule) && rule.dot->is_terminal && rule.dot->val.terminal(token)) {
            struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
            *to_append = (struct earley_rule) {
                .lhs=rule.lhs, .rhs=rule.rhs, .dot=rule.dot+1, .origin_chart=rule.origin_chart, .completed_from=rule.completed_from
            };
            erule_p_vec_append(out, to_append);
        }
    }
    // Complete and predict in a loop
    for (size_t i=0; i < out->n_elements; i++) {
        struct earley_rule rule = *out->arr[i];
        // Complete
        if (is_completed(rule)) {
            for (size_t j=0; j < rule.origin_chart->n_elements; j++) {
                struct earley_rule origin_rule = *rule.origin_chart->arr[j];
                if (!origin_rule.dot->is_terminal && !is_completed(origin_rule) && origin_rule.dot->val.rule == rule.lhs) {
                    erule_p_vec new_completed_from;
                    erule_p_vec_init(&new_completed_from, origin_rule.completed_from.n_elements+1);
                    erule_p_vec_append_all(&new_completed_from, &origin_rule.completed_from);
                    struct earley_rule *to_append = MALLOC(sizeof(struct earley_rule));
                    *to_append = (struct earley_rule) {
                        .lhs=origin_rule.lhs, .rhs=origin_rule.rhs, .dot=origin_rule.dot+1,
                        .origin_chart=origin_rule.origin_chart, .completed_from=new_completed_from
                    };
                    erule_p_vec_append(out, to_append);
                }
            }
        }
        // Predict
        recursively_predict(rule, out, out);
    }

    return out;
}


static void print_symbol(struct symbol sym) {
    if (sym.is_terminal) {
        printf("Terminal ");
    } else {
        printf("%s ", sym.val.rule->name);
    }
}

static void print_chart(erule_p_vec *chart) {
    for (size_t i = 0; i < chart->n_elements; i++) {
        struct earley_rule *rule = chart->arr[i];
        printf("%s -> ", rule->lhs->name);
        for (size_t j = 0; j < rule->rhs.n; j++) {
            if (rule->dot == &rule->rhs.symbols[j]) {
                printf("[dot] ");
            }
            print_symbol(rule->rhs.symbols[j]);
        }
        if (rule->dot == rule->rhs.symbols + rule->rhs.n) {
            printf("[dot]");
        }
        printf("\n");
    }
}



void test_next_chart(pp_token_vec tokens) {
    // Initialize the starting chart with the start symbol 'S'
    // erule_p_vec initial_chart;
    // erule_p_vec_init(&initial_chart, 1);
    // erule_p_vec empty;
    // erule_p_vec_init(&empty, 0);
    //
    // struct earley_rule *S_to_AB = MALLOC(sizeof(struct earley_rule));
    //
    // *S_to_AB = (struct earley_rule){
    //     .lhs=&rule_S, .rhs=rule_S.alternatives[0], .dot=rule_S.alternatives[0].symbols, .origin_chart=&initial_chart, .completed_from=empty
    // };
    // erule_p_vec_append(&initial_chart, S_to_AB);
    //
    // printf("Initial Chart:\n");
    // print_chart(&initial_chart);
    //
    // recursively_predict(*S_to_AB, &initial_chart, &initial_chart);
    // printf("Amended Initial Chart:\n");
    // print_chart(&initial_chart);
    //
    // erule_p_vec *old_chart = &initial_chart;
    // erule_p_vec *new_chart = &initial_chart;
    // for (size_t i = 0; i < tokens.n_elements; i++) {
    //     printf("Chart after processing token %zu:\n", i);
    //     new_chart = next_chart(old_chart, tokens.arr[i]);
    //     print_chart(new_chart);
    // }
}
