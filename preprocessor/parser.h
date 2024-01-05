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

struct production_rhs {
    struct parsing_symbol **symbols;
    size_t n_symbols;
};

struct parsing_symbol {
    union {
        enum detection_status (*match_fn)(const char *chars, size_t n_chars);
        struct match_list {
            struct production_rhs *matches;
            size_t n_matches;
        } match_list;
    } val;
    enum { FN, LIST } fn_or_list;
};

typedef struct parsing_symbol parsing_rule;
DEFINE_VEC_TYPE_AND_FUNCTIONS(parsing_rule)
parsing_rule_vec get_simple_matches(const char *chars, size_t n_chars);

typedef struct production_rhs rule_sequence;
DEFINE_VEC_TYPE_AND_FUNCTIONS(rule_sequence)

#endif //ICK_PARSER_H
