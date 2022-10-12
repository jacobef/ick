//
// Created by Jacob Friedman on 10/6/22.
//

#ifndef ICK_PARSER_H
#define ICK_PARSER_H

#include "data_structures/linked_list.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum detection_status {
    MATCH, INCOMPLETE, IMPOSSIBLE
};

struct rule_match {
    struct parsing_rule **chained_rules;
    size_t n_rules;
};

struct parsing_rule {
    union {
        enum detection_status (*match_fn)(const char *chars, size_t n_chars);
        struct {
            struct rule_match *matches;
            size_t n_matches;
        } match_list;
    } val;
    enum { FN, LIST } fn_or_list;
};

static enum detection_status detect_nonzero_digit(const char *chars, size_t n_chars) {
    if (n_chars == 1 && isdigit(chars[0]) && chars[0] != '\0') return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}

static enum detection_status detect_digit(const char *chars, size_t n_chars) {
    if (n_chars == 1 && isdigit(chars[0])) return MATCH;
    else if (n_chars == 0) return INCOMPLETE;
    else return IMPOSSIBLE;
}
enum detection_status check_single_match(struct rule_match match, const char *chars, size_t n_chars);
enum detection_status check_rule(struct parsing_rule *rule, const char *chars, size_t n_chars);

static struct parsing_rule nonzero_digit = {
        .fn_or_list=FN, .val.match_fn = detect_nonzero_digit
};
static struct parsing_rule digit = {
        .fn_or_list=FN, .val.match_fn = detect_digit
};
static struct parsing_rule decimal_constant = {
        .fn_or_list=LIST, .val.match_list = {
                .n_matches=2, .matches = (struct rule_match[]) {
                    (struct rule_match) {
                        .n_rules=1, .chained_rules = (struct parsing_rule*[]) {
                            &nonzero_digit
                        }
                    },
                    (struct rule_match) {
                        .n_rules=2, .chained_rules = (struct parsing_rule*[]) {
                            &decimal_constant, &digit
                        }
                    }
                }
        }
};

#endif //ICK_PARSER_H
