//
// Created by Jacob Friedman on 10/6/22.
//

#include "parser.h"
#include "diagnostics.h"

enum detection_status check_single_match(struct rule_match match, const char *chars, size_t n_chars) {
    size_t chars_i = 0;
    for (size_t rules_i = 0; rules_i < match.n_rules; rules_i++) {
        struct parsing_rule *rule = match.chained_rules[rules_i];
        size_t max_n_chars_that_match_rule;
        bool found_match = false;
        for (size_t n_chars_to_check = 0; n_chars_to_check <= n_chars-chars_i; n_chars_to_check++) {
            enum detection_status rule_match_status = check_rule(rule, chars + chars_i, n_chars_to_check);
            if (rule_match_status == MATCH) {
                max_n_chars_that_match_rule = n_chars_to_check;
                found_match = true;
            }
            else if (rule_match_status == IMPOSSIBLE && found_match) {
                break;
            }
            else if (rule_match_status == IMPOSSIBLE && n_chars-chars_i == 0) {
                return INCOMPLETE;
            }
            else if (rule_match_status == IMPOSSIBLE) {
                return IMPOSSIBLE;
            }
        }
        chars_i += max_n_chars_that_match_rule;
    }
    return MATCH;
}


enum detection_status check_rule(struct parsing_rule *rule, const char *chars, size_t n_chars) {
    if (rule->fn_or_list == FN) {
        return rule->val.match_fn(chars, n_chars);
    }
    else if (rule->fn_or_list == LIST) {
        enum detection_status best_status = IMPOSSIBLE;
        for (size_t i = 0; i < rule->val.match_list.n_matches; i++) {
            enum detection_status match_status = check_single_match(rule->val.match_list.matches[i], chars, n_chars);
            if (match_status == MATCH) {
                best_status = MATCH;
            }
            else if (match_status == INCOMPLETE && best_status == IMPOSSIBLE) {
                best_status = INCOMPLETE;
            }
        }
        return best_status;
    }
    else {
        preprocessor_error(0, 0, 0,
                           "internal error: fn_or_list is not FN or LIST");
    }
}
