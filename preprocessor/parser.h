//
// Created by Jacob Friedman on 10/6/22.
//

#ifndef ICK_PARSER_H
#define ICK_PARSER_H

#include "data_structures/vector.h"
#include "detector.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

struct rule_sequence {
    struct parsing_rule **chained_rules;
    size_t n_rules;
};

struct parsing_rule {
    union {
        enum detection_status (*match_fn)(const char *chars, size_t n_chars);
        struct match_list {
            struct rule_sequence *matches;
            size_t n_matches;
        } match_list;
    } val;
    enum { FN, LIST } fn_or_list;
};

typedef struct parsing_rule parsing_rule;
DEFINE_VEC_TYPE_AND_FUNCTIONS(parsing_rule)
parsing_rule_vec get_simple_matches(const char *chars, size_t n_chars);

typedef struct rule_sequence rule_sequence;
DEFINE_VEC_TYPE_AND_FUNCTIONS(rule_sequence)
rule_sequence_vec get_sequences_that_start_with(struct rule_sequence start_with, struct rule_sequence *possible_supersequences,
                              size_t n_possible_supersequences);

enum detection_status check_single_match(struct rule_sequence match, const char *chars, size_t n_chars);
enum detection_status check_rule(struct parsing_rule *rule, const char *chars, size_t n_chars);

parsing_rule_vec get_rules_that_have(struct rule_sequence sequence);

void test_thing(void);

#endif //ICK_PARSER_H
