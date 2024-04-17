#include <stdbool.h>
#include "preprocessor/char_parser.h"

#define ALT(_tag, ...)                                                              \
    ((const struct char_alternative) {                                              \
        .symbols=(struct char_symbol[]) {__VA_ARGS__},                              \
        .n=sizeof((struct char_symbol[]){__VA_ARGS__})/sizeof(struct char_symbol),  \
        .tag=_tag                                                                   \
    })

#define PR_RULE(_name, ...)                                                                 \
    ((const struct char_production_rule) {                                                  \
        .name=_name,                                                                        \
        .alternatives=(const struct char_alternative[]) {__VA_ARGS__},                      \
        .n=sizeof((struct char_alternative[]){__VA_ARGS__})/sizeof(struct char_alternative) \
    })

#define NT_SYM(_rule)       \
    ((struct char_symbol) { \
        .val.rule=&(_rule), \
        .is_terminal=false  \
    })

#define T_SYM_FN(_fn)          \
    ((struct char_symbol) {    \
        .val.terminal = {      \
            .matcher.fn=_fn,   \
            .type=CHAR_TERMINAL_FN, \
            .is_filled=false   \
        },                     \
        .is_terminal=true      \
    })

#define T_SYM_CHAR(_chr)                        \
    ((struct char_symbol) {                         \
        .val.terminal = {                      \
            .matcher.chr=_chr, \
            .type=CHAR_TERMINAL_CHAR,                \
            .is_filled=false                   \
        },                                     \
        .is_terminal=true,                     \
    })

#define EMPTY_ALT(_tag)                   \
    ((struct char_alternative) {               \
        .symbols=(struct char_symbol[]) { 0 }, \
        .n=0,                             \
        .tag=_tag                         \
    })

#define OPT(_name, _rule) PR_RULE(_name, ALT(OPT_ONE, NT_SYM(_rule)), EMPTY_ALT(OPT_NONE))

static bool match_non_esc_c_char(unsigned char c) {
    return c != '\'' && c != '\\' && c != '\n';
}

static bool match_octal_digit(unsigned char c) {
    return '0' <= c && c <= '7';
}

static bool match_hex_digit(unsigned char c) {
    return isxdigit(c);
}

#define NO_TAG -1

const struct char_production_rule cr_character_constant = PR_RULE("character-constant",
        ALT(CHARACTER_CONSTANT_NORMAL, T_SYM_CHAR('\''), NT_SYM(cr_c_char_sequence), T_SYM_CHAR('\'')),
        ALT(CHARACTER_CONSTANT_WIDE, T_SYM_CHAR('L'), T_SYM_CHAR('\''), NT_SYM(cr_c_char_sequence), T_SYM_CHAR('\'')));

const struct char_production_rule cr_c_char_sequence = PR_RULE("c-char-sequence",
                                                               ALT(CR_LIST_RULE_ONE, NT_SYM(cr_c_char)),
                                                               ALT(CR_LIST_RULE_MULTI, NT_SYM(cr_c_char_sequence), NT_SYM(cr_c_char)));

const struct char_production_rule cr_c_char = PR_RULE("c-char",
                                                      ALT(C_CHAR_NOT_ESCAPE_SEQUENCE, T_SYM_FN(match_non_esc_c_char)),
                                                      ALT(C_CHAR_ESCAPE_SEQUENCE, NT_SYM(cr_escape_sequence)));

const struct char_production_rule cr_escape_sequence = PR_RULE("escape-sequence",
                                                               ALT(ESCAPE_SEQUENCE_SIMPLE, NT_SYM(cr_simple_escape_sequence)),
                                                               ALT(ESCAPE_SEQUENCE_OCTAL, NT_SYM(cr_octal_escape_sequence)),
                                                               ALT(ESCAPE_SEQUENCE_HEX, NT_SYM(cr_hex_escape_sequence)),
                                                               ALT(ESCAPE_SEQUENCE_UNIVERSAL, NT_SYM(cr_universal_character_name)));

const struct char_production_rule cr_simple_escape_sequence = PR_RULE("simple-escape-sequence",
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_SINGLE_QUOTE, T_SYM_CHAR('\\'), T_SYM_CHAR('\'')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_DOUBLE_QUOTE, T_SYM_CHAR('\\'), T_SYM_CHAR('\"')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_QUESTION_MARK, T_SYM_CHAR('\\'), T_SYM_CHAR('?')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_BACKSLASH, T_SYM_CHAR('\\'), T_SYM_CHAR('\\')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_ALERT, T_SYM_CHAR('\\'), T_SYM_CHAR('a')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_BACKSPACE, T_SYM_CHAR('\\'), T_SYM_CHAR('b')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_FORM_FEED, T_SYM_CHAR('\\'), T_SYM_CHAR('f')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_NEWLINE, T_SYM_CHAR('\\'), T_SYM_CHAR('n')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_CARRIAGE_RETURN, T_SYM_CHAR('\\'), T_SYM_CHAR('r')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_HORIZONTAL_TAB, T_SYM_CHAR('\\'), T_SYM_CHAR('t')),
                                                                      ALT(SIMPLE_ESCAPE_SEQUENCE_VERTICAL_TAB, T_SYM_CHAR('\\'), T_SYM_CHAR('v')));

const struct char_production_rule cr_octal_escape_sequence = PR_RULE("octal-escape-sequence",
                                                                     ALT(OCTAL_ESCAPE_SEQUENCE_1_DIGIT, T_SYM_CHAR('\\'), NT_SYM(cr_octal_digit)),
                                                                     ALT(OCTAL_ESCAPE_SEQUENCE_2_DIGIT, T_SYM_CHAR('\\'), NT_SYM(cr_octal_digit), NT_SYM(cr_octal_digit)),
                                                                     ALT(OCTAL_ESCAPE_SEQUENCE_3_DIGIT, T_SYM_CHAR('\\'), NT_SYM(cr_octal_digit), NT_SYM(cr_octal_digit), NT_SYM(cr_octal_digit)));

const struct char_production_rule cr_octal_digit = PR_RULE("octal-digit", ALT(NO_TAG, T_SYM_FN(match_octal_digit)));

const struct char_production_rule cr_hex_escape_sequence = PR_RULE("hexadecimal-escape-sequence",
                                                                   ALT(CR_LIST_RULE_ONE, T_SYM_CHAR('\\'), T_SYM_CHAR('x'), NT_SYM(cr_hexadecimal_digit)),
                                                                   ALT(CR_LIST_RULE_MULTI, NT_SYM(cr_hex_escape_sequence), NT_SYM(cr_hexadecimal_digit)));

const struct char_production_rule cr_hexadecimal_digit = PR_RULE("hexadecimal-digit", ALT(NO_TAG, T_SYM_FN(match_hex_digit)));

const struct char_production_rule cr_universal_character_name = PR_RULE("universal-character-name",
                                                                        ALT(UNIVERSAL_CHARACTER_NAME_4_DIGIT, T_SYM_CHAR('\\'), T_SYM_CHAR('u'), NT_SYM(cr_hex_quad)),
                                                                        ALT(UNIVERSAL_CHARACTER_NAME_8_DIGIT, T_SYM_CHAR('\\'), T_SYM_CHAR('U'), NT_SYM(cr_hex_quad), NT_SYM(cr_hex_quad)));

const struct char_production_rule cr_hex_quad = PR_RULE("hex-quad", ALT(NO_TAG, NT_SYM(cr_hexadecimal_digit), NT_SYM(cr_hexadecimal_digit), NT_SYM(cr_hexadecimal_digit), NT_SYM(cr_hexadecimal_digit)));
