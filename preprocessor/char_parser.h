#ifndef ICK_CHAR_PARSER_H
#define ICK_CHAR_PARSER_H

#include "data_structures/vector.h"
#include "preprocessor/detector.h"
#include "preprocessor/pp_token.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum char_terminal_symbol_type {
    CHAR_TERMINAL_FN, CHAR_TERMINAL_CHAR
};

struct char_terminal {
    union {
        bool (*fn)(unsigned char);
        unsigned char chr;
    } matcher;
    enum char_terminal_symbol_type type;
    unsigned char token;
    bool is_filled;
};

struct char_symbol {
    union {
        const struct char_production_rule *rule;
        struct char_terminal terminal;
    } val;
    bool is_terminal;
};

struct char_alternative {
    struct char_symbol *symbols;
    size_t n;
    int tag;
};

struct char_production_rule {
    const char *name;
    const struct char_alternative *alternatives;
    const size_t n;
};

typedef struct char_earley_rule *char_erule_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(char_erule_p)

struct char_earley_rule {
    const struct char_production_rule *lhs;
    struct char_alternative rhs;
    struct char_symbol *dot;
    char_erule_p_vec *origin_chart;
    char_erule_p_vec completed_from;
};

typedef char_erule_p_vec *char_erule_p_vec_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(char_erule_p_vec_p)
typedef unsigned char uchar;
DEFINE_VEC_TYPE_AND_FUNCTIONS(uchar)
char_erule_p_vec_p_vec char_make_charts(uchar_vec tokens, const struct char_production_rule *root_rule);

void char_print_chart(char_erule_p_vec *chart);
void char_print_tree(struct char_earley_rule *root, size_t indent);
struct char_earley_rule *char_parse(uchar_vec tokens, const struct char_production_rule *root_rule);
void char_test_parser(uchar_vec tokens);

enum cr_list_rule_tag {
    CR_LIST_RULE_ONE,
    CR_LIST_RULE_MULTI
};

enum character_constant_tag {
    CHARACTER_CONSTANT_NORMAL,
    CHARACTER_CONSTANT_WIDE
};

enum c_char_tag {
    C_CHAR_NOT_ESCAPE_SEQUENCE,
    C_CHAR_ESCAPE_SEQUENCE
};

enum escape_sequence_tag {
    ESCAPE_SEQUENCE_SIMPLE,
    ESCAPE_SEQUENCE_OCTAL,
    ESCAPE_SEQUENCE_HEX,
    ESCAPE_SEQUENCE_UNIVERSAL
};

enum simple_escape_sequence_tag {
    SIMPLE_ESCAPE_SEQUENCE_SINGLE_QUOTE,
    SIMPLE_ESCAPE_SEQUENCE_DOUBLE_QUOTE,
    SIMPLE_ESCAPE_SEQUENCE_QUESTION_MARK,
    SIMPLE_ESCAPE_SEQUENCE_BACKSLASH,
    SIMPLE_ESCAPE_SEQUENCE_ALERT,
    SIMPLE_ESCAPE_SEQUENCE_BACKSPACE,
    SIMPLE_ESCAPE_SEQUENCE_FORM_FEED,
    SIMPLE_ESCAPE_SEQUENCE_NEWLINE,
    SIMPLE_ESCAPE_SEQUENCE_CARRIAGE_RETURN,
    SIMPLE_ESCAPE_SEQUENCE_HORIZONTAL_TAB,
    SIMPLE_ESCAPE_SEQUENCE_VERTICAL_TAB
};

enum octal_escape_sequence_tag {
    OCTAL_ESCAPE_SEQUENCE_1_DIGIT,
    OCTAL_ESCAPE_SEQUENCE_2_DIGIT,
    OCTAL_ESCAPE_SEQUENCE_3_DIGIT
};

enum universal_character_name_tag {
    UNIVERSAL_CHARACTER_NAME_4_DIGIT,
    UNIVERSAL_CHARACTER_NAME_8_DIGIT
};

extern const struct char_production_rule cr_character_constant;
extern const struct char_production_rule cr_c_char_sequence;
extern const struct char_production_rule cr_c_char;
extern const struct char_production_rule cr_escape_sequence;
extern const struct char_production_rule cr_simple_escape_sequence;
extern const struct char_production_rule cr_octal_escape_sequence;
extern const struct char_production_rule cr_octal_digit;
extern const struct char_production_rule cr_hex_escape_sequence;
extern const struct char_production_rule cr_hexadecimal_digit;
extern const struct char_production_rule cr_universal_character_name;
extern const struct char_production_rule cr_hex_quad;

#endif //ICK_CHAR_PARSER_H
