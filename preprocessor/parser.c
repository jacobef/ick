//
// Created by Jacob Friedman on 10/6/22.
//

#include "parser.h"
#include "diagnostics.h"
#include "debug/malloc.h"

static enum detection_status detect_nonzero_digit(const char *chars, size_t n_chars) {
    if (n_chars == 1 && isdigit(chars[0]) && chars[0] != '0') return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}

static enum detection_status detect_digit(const char *chars, size_t n_chars) {
    if (n_chars == 1 && isdigit(chars[0])) return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}

static enum detection_status detect_octal_digit(const char *chars, size_t n_chars) {
    if (n_chars == 1 && chars[0] >= '0' && chars[0] < '8') return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}

static enum detection_status detect_zero(const char *chars, size_t n_chars) {
    if (n_chars == 1 && chars[0] == '0') return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}

static struct parsing_rule nonzero_digit = {
        .fn_or_list=FN, .val.match_fn = detect_nonzero_digit
};
static struct parsing_rule digit = {
        .fn_or_list=FN, .val.match_fn = detect_digit
};
static struct parsing_rule decimal_constant = {
        .fn_or_list=LIST, .val.match_list = {
                .n_matches=2, .matches = (struct rule_sequence[]) {
                        (struct rule_sequence) {
                                .n_rules=1, .chained_rules = (struct parsing_rule*[]) {
                                        &nonzero_digit
                                }
                        },
                        (struct rule_sequence) {
                                .n_rules=2, .chained_rules = (struct parsing_rule*[]) {
                                        &decimal_constant, &digit
                                }
                        }
                }
        }
};
static struct parsing_rule octal_digit = {
        .fn_or_list=FN, .val.match_fn = detect_octal_digit
};
static struct parsing_rule zero = {
        .fn_or_list=FN, .val.match_fn = detect_zero
};
static struct parsing_rule octal_constant = {
        .fn_or_list=LIST, .val.match_list = {
                .n_matches = 2, .matches = (struct rule_sequence[]) {
                        (struct rule_sequence) {
                                .n_rules=1, .chained_rules = (struct parsing_rule *[]) {
                                        &zero
                                }
                        },
                        (struct rule_sequence) {
                                .n_rules=2, .chained_rules = (struct parsing_rule *[]) {
                                        &zero, &octal_constant
                                }
                        }
                }
        }
};

static struct parsing_rule *all_parsing_rules[] = {&nonzero_digit, &digit, &decimal_constant, &octal_digit, &zero, &octal_constant};
static size_t n_all_parsing_rules = sizeof(all_parsing_rules) / sizeof(all_parsing_rules[0]);


static parsing_rule_vec get_simple_parsing_rules(enum detection_status match_level, const char *chars, size_t n_chars) {
    parsing_rule_vec result;
    parsing_rule_vec_init(&result, 0);
    for (size_t i = 0; i < n_all_parsing_rules; i++) {
        if (all_parsing_rules[i]->fn_or_list == FN && all_parsing_rules[i]->val.match_fn(chars, n_chars) == match_level) {
            parsing_rule_vec_append(&result, *all_parsing_rules[i]);
        }
    }
    return result;
}

static bool is_same_sequence(struct rule_sequence seq1, struct rule_sequence seq2) {
    if (seq1.n_rules != seq2.n_rules) return false;
    for (size_t i = 0; i < seq1.n_rules; i++) {
        if (seq1.chained_rules[i] != seq2.chained_rules[i]) return false;
    }
    return true;
}

static bool rule_has_sequence(struct parsing_rule rule, struct rule_sequence sequence) {
    if (rule.fn_or_list == FN) return false;
    struct match_list match_list = rule.val.match_list;
    for (size_t j = 0; j < match_list.n_matches; j++) {
        if (is_same_sequence(match_list.matches[j], sequence)) {
            return true;
        }
    }
    return false;
}

parsing_rule_vec get_rules_that_have(struct rule_sequence sequence) {
    parsing_rule_vec result;
    parsing_rule_vec_init(&result, 0);
    for (size_t i = 0; i < n_all_parsing_rules; i++) {
        if (rule_has_sequence(*all_parsing_rules[i], sequence)) {
            parsing_rule_vec_append(&result, *all_parsing_rules[i]);
        }
    }
    return result;
}

static parsing_rule_vec get_equivalent_rules(struct rule_sequence sequence) {
    parsing_rule_vec result = get_rules_that_have(sequence);
    for (size_t i = 0; i < result.n_elements; i++) {
        struct rule_sequence one_rule_sequence = { .chained_rules = (struct parsing_rule*[]) { &result.arr[i] }, .n_rules = 1 };
        parsing_rule_vec parent_rules = get_equivalent_rules(one_rule_sequence);
        parsing_rule_vec_append_all(&result, &parent_rules);
        parsing_rule_vec_free_internals(&parent_rules);
    }
    return result;
}

void test_thing(void) {
    __attribute__((unused)) parsing_rule_vec a = get_equivalent_rules((struct rule_sequence) {
            .n_rules=1, .chained_rules = (struct parsing_rule *[]) {
                    &zero
            }
    });
}

//rule_sequence_vec get_equivalent_sequences(struct rule_sequence sequence) {
//    rule_sequence_vec result;
//    rule_sequence_vec_init(&result, 0);
//}


rule_sequence_vec get_sequences_that_start_with(struct rule_sequence start_with,
        struct rule_sequence *possible_supersequences, size_t n_possible_supersequences) {
    rule_sequence_vec result;
    rule_sequence_vec_init(&result, 0);
    for (size_t i = 0; i < n_possible_supersequences; i++) {
        if (start_with.n_rules > possible_supersequences[i].n_rules) continue;
        bool matched = true;
        for (size_t j = 0; j < start_with.n_rules; j++) {
            if (possible_supersequences[i].chained_rules[j] != start_with.chained_rules[j]) {
                matched = false;
            }
        }
        if (matched) rule_sequence_vec_append(&result, possible_supersequences[i]);
    }
    return result;
}

//struct parsing_rule get_parsing_rule_given(struct rule_sequence current_rule_sequence,
//        char *remaining_chars, size_t n_remaining_chars) {
//    struct parsing_rule *last_rule = current_rule_sequence.chained_rules[current_rule_sequence.n_rules-1];
//}

//struct parsing_rule get_parsing_rule(char *chars, size_t n_chars) {
//    parsing_rule_vec current_rule_sequence;
//    parsing_rule_vec_init(&current_rule_sequence, 0);
//
//    parsing_rule_vec simple_rule_matches = get_simple_parsing_rules(MATCH, chars, n_chars);
//    for (size_t i = 0; i < simple_rule_matches.n_elements; i++) {
//        struct rule_sequence new_rule_sequence;
//        new_rule_sequence.n_rules = current_rule_sequence.n_rules + 1;
//        new_rule_sequence.chained_rules = alloca(sizeof(new_rule_sequence.chained_rules[0]) * new_rule_sequence.n_rules);
//        memcpy(new_rule_sequence.chained_rules, current_rule_sequence.chained_rules, current_rule_sequence.n_rules);
//        new_rule_sequence.chained_rules[new_rule_sequence.n_rules-1] = &simple_rule_matches.arr[i];
//    }
//    }
//}
