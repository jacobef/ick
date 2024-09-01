#include <stdbool.h>
#include "preprocessor/parser.h"

#define ALT(_tag, ...)                                                   \
    ((const struct alternative) {                                        \
        .symbols=(struct symbol[]) {__VA_ARGS__},                        \
        .n=sizeof((struct symbol[]){__VA_ARGS__})/sizeof(struct symbol), \
        .tag=_tag                                                        \
    })

#define PR_RULE(_name, _is_list_rule, ...)                               \
    ((const struct production_rule) {                                    \
        .name=_name,                                                     \
        .alternatives=(const struct alternative[]) {__VA_ARGS__},        \
        .n=sizeof((struct alternative[]){__VA_ARGS__})/sizeof(struct alternative), \
        .is_list_rule=_is_list_rule                                      \
    })

#define NT_SYM(_rule)       \
    ((struct symbol) {      \
        .val.rule=&(_rule), \
        .is_terminal=false  \
    })

#define T_SYM_FN(_fn)          \
    ((struct symbol) {         \
        .val.terminal = {      \
            .matcher.fn=_fn,   \
            .type=TERMINAL_FN, \
            .is_filled=false   \
        },                     \
        .is_terminal=true      \
    })

#define T_SYM_STR(_str)                        \
    ((struct symbol) {                         \
        .val.terminal = {                      \
            .matcher.str=(unsigned char*)_str, \
            .type=TERMINAL_STR,                \
            .is_filled=false                   \
        },                                     \
        .is_terminal=true,                     \
    })

#define EMPTY_ALT(_tag)                   \
    ((struct alternative) {               \
        .symbols=(struct symbol[]) { 0 }, \
        .n=0,                             \
        .tag=_tag                         \
    })

#define OPT(_name, _rule) PR_RULE(_name, false, ALT(OPT_ONE, NT_SYM(_rule)), EMPTY_ALT(OPT_NONE))

static bool match_preprocessing_token(__attribute__((unused)) struct preprocessing_token token) {
    return !token_is_str(token, "\n");
}

static bool match_lparen(struct preprocessing_token token) {
    return token_is_str(token, "(") && !token.after_whitespace;
}

static bool match_identifier(struct preprocessing_token token) {
    return token.type == IDENTIFIER;
}

static bool match_string_literal(struct preprocessing_token token) {
    return token.type == STRING_LITERAL;
}

static bool match_character_constant(struct preprocessing_token token) {
    return token.type == CHARACTER_CONSTANT;
}

static bool match_non_hashtag(struct preprocessing_token token) {
    return match_preprocessing_token(token) && !token_is_str(token, "#");
}

static bool match_non_directive_name(struct preprocessing_token token) {
    return !token_is_str(token, "define") &&
           !token_is_str(token, "undef") &&
           !token_is_str(token, "if") &&
           !token_is_str(token, "ifdef") &&
           !token_is_str(token, "ifndef") &&
           !token_is_str(token, "elif") &&
           !token_is_str(token, "else") &&
           !token_is_str(token, "endif") &&
           !token_is_str(token, "include") &&
           !token_is_str(token, "line") &&
           !token_is_str(token, "error") &&
           !token_is_str(token, "pragma");
}

static bool is_int_suffix(struct str_view str_view) {
    char *suffixes[] = {
            "u", "U", "l", "L",
            "ll", "LL",
            "ul", "uL", "Ul", "UL",
            "lu", "lU", "Lu", "LU",
            "llu", "llU", "lLu", "LLu", "LLU",
            "ull", "uLL", "Ull", "ULL"
    };
    for (size_t i = 0; i < sizeof(suffixes)/sizeof(char*); i++) {
        if (str_view_cstr_eq(str_view, suffixes[i])) return true;
    }
    return false;
}

static bool match_integer_constant(struct preprocessing_token token) {
    if (token.name.n == 0) return false;
    else if (token.name.n == 1) return token.name.chars[0] >= '0' && token.name.chars[0] <= '9';
    else {
        size_t i;
        if (token.name.chars[0] == '0' && (token.name.chars[1] == 'x' || token.name.chars[1] == 'X')) {
            for (i = 2; i < token.name.n && isxdigit(token.name.chars[i]); i++);
            if (i == 2) return false;
        } else if (token.name.chars[0] == '0') {
            for (i = 1; i < token.name.n && token.name.chars[i] >= '0' && token.name.chars[i] <= '7'; i++);
        } else if (isdigit(token.name.chars[0])) {
            for (i = 1; i < token.name.n && token.name.chars[i] >= '0' && token.name.chars[i] <= '9'; i++);
        } else {
            return false;
        }
        return i == token.name.n || is_int_suffix((struct str_view) { .chars = &token.name.chars[i], .n = token.name.n - i });
    }
}

static ssize_t scan_digit_sequence(struct str_view str_view, size_t i) {
    if (i >= str_view.n || !isdigit(str_view.chars[i])) return -1;
    for (; i < str_view.n && isdigit(str_view.chars[i]); i++);
    return (ssize_t) i;
}

static ssize_t scan_fractional_constant(struct str_view str_view, size_t i) {
    // shortest possible fractional constant is .0 or 0.
    if (i > str_view.n-2) return -1;
    if (str_view.chars[i] == '.') {
        i++;
        // 1 or more digits must follow the dot
        if (i >= str_view.n || !isdigit(str_view.chars[i])) return -1;
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.chars[i]); i++);
        return (ssize_t) i;
    } else if (isdigit(str_view.chars[i])) {
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.chars[i]); i++);
        // if we reach the end first, then there's no dot
        if (i >= str_view.n || str_view.chars[i] != '.') return -1;
        // scan past the dot
        i++;
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.chars[i]); i++);
        return (ssize_t) i;
    } else {
        return -1;
    }
}

static ssize_t scan_exponent_part(struct str_view str_view, size_t i) {
    // shortest possible exponent part is e0 or E0
    if (i > str_view.n-2 || (str_view.chars[i] != 'e' && str_view.chars[i] != 'E')) return -1;
    i++;
    if (i >= str_view.n) return -1;
    // optional sign
    if (str_view.chars[i] == '+' || str_view.chars[i] == '-') i++;
    // digit sequence required at the end
    if (i >= str_view.n || !isdigit(str_view.chars[i])) return -1;
    // scan until we reach a non-digit or the end
    for (; i < str_view.n && isdigit(str_view.chars[i]); i++);
    return (ssize_t) i;
}

static bool is_floating_suffix(unsigned char c) {
    return c == 'f' || c == 'F' || c == 'l' || c == 'L';
}

static bool match_decimal_floating_constant(struct preprocessing_token token) {
    const ssize_t fi = scan_fractional_constant(token.name, 0);
    const ssize_t di = scan_digit_sequence(token.name, 0);
    if (fi == -1 && di == -1) return false;
    else if (fi != -1) { // fractional-constant exponent-part_opt floating-suffix_opt
        const ssize_t ei = scan_exponent_part(token.name, (size_t) fi);
        if (ei == -1) { // exponent part doesn't exist
            // should be a floating suffix or the end
            return (size_t)fi == token.name.n || ((size_t)fi+1 == token.name.n && is_floating_suffix(token.name.chars[fi]));
        } else { // exponent part does exist
            // should be a floating suffix or the end
            return (size_t)ei == token.name.n || ((size_t)ei+1 == token.name.n && is_floating_suffix(token.name.chars[ei]));
        }
    } else { // digit-sequence exponent-part floating-suffix_opt
        const ssize_t ei = scan_exponent_part(token.name, (size_t) di);
        if (ei == -1) return false;
        return (size_t) ei == token.name.n || ((size_t)ei+1 == token.name.n && is_floating_suffix(token.name.chars[ei]));
    }
}

static bool match_floating_constant(struct preprocessing_token token) {
    // TODO add hex float support
    return match_decimal_floating_constant(token);
}

#define NO_TAG (-1)

// preprocessing-file: group_opt
const struct production_rule tr_preprocessing_file = PR_RULE("preprocessing-file", false, ALT(NO_TAG, NT_SYM(tr_group_opt)));

// group: group-part group group-part
const struct production_rule tr_group = PR_RULE("group", true,
                                                ALT(LIST_RULE_ONE, NT_SYM(tr_group_part)),
                                                ALT(LIST_RULE_MULTI, NT_SYM(tr_group), NT_SYM(tr_group_part)));
const struct production_rule tr_group_opt = OPT("group_opt", tr_group);

// group-part: if-section control-line text-line # non-directive
const struct production_rule tr_group_part = PR_RULE("group-part", false,
                                                     ALT(GROUP_PART_IF, NT_SYM(tr_if_section)),
                                                     ALT(GROUP_PART_CONTROL, NT_SYM(tr_control_line)),
                                                     ALT(GROUP_PART_TEXT, NT_SYM(tr_text_line)),
                                                     ALT(GROUP_PART_NON_DIRECTIVE, T_SYM_STR("#"), NT_SYM(tr_non_directive)));

// if-section: if-group elif-groups_opt else-group_opt endif-line
const struct production_rule tr_if_section = PR_RULE("if-section", false,
                                                     ALT(NO_TAG, NT_SYM(tr_if_group), NT_SYM(tr_elif_groups_opt), NT_SYM(tr_else_group_opt), NT_SYM(tr_endif_line)));

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
const struct production_rule tr_if_group = PR_RULE("if-group", false,
                                                   ALT(IF_GROUP_IF, T_SYM_STR("#"), T_SYM_STR("if"), NT_SYM(tr_constant_expression), T_SYM_STR("\n"), NT_SYM(tr_group_opt)),
                                                   ALT(IF_GROUP_IFDEF, T_SYM_STR("#"), T_SYM_STR("ifdef"), NT_SYM(tr_identifier), T_SYM_STR("\n"), NT_SYM(tr_group_opt)),
                                                   ALT(IF_GROUP_IFNDEF, T_SYM_STR("#"), T_SYM_STR("ifndef"), NT_SYM(tr_identifier), T_SYM_STR("\n"), NT_SYM(tr_group_opt)));

// elif-groups: elif-group
//              elif-groups elif-group
const struct production_rule tr_elif_groups = PR_RULE("elif_groups", true,
                                                      ALT(LIST_RULE_ONE, NT_SYM(tr_elif_group)),
                                                      ALT(LIST_RULE_MULTI, NT_SYM(tr_elif_groups), NT_SYM(tr_elif_group)));
const struct production_rule tr_elif_groups_opt = OPT("elif-groups_opt", tr_elif_groups);

// elif-group: # elif constant-expression new-line group_opt
const struct production_rule tr_elif_group = PR_RULE("elif_group", false,
                                                     ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("elif"), NT_SYM(tr_constant_expression), T_SYM_STR("\n"), NT_SYM(tr_group_opt)));

const struct production_rule tr_else_group = PR_RULE("else-group", false,
                                                     ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("else"), T_SYM_STR("\n"), NT_SYM(tr_group_opt)));
const struct production_rule tr_else_group_opt = OPT("else-group_opt", tr_else_group);

// endif-line: # endif new-line
const struct production_rule tr_endif_line = PR_RULE("endif-line", false,
                                                     ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("endif"), T_SYM_STR("\n")));

// control-line:
// # include pp-tokens new-line
// # define identifier replacement-list new-line
// # define identifier lparen identifier-list_opt ) replacement-list new-line
// # define identifier lparen ... ) replacement-list new-line
// # define identifier lparen identifier-list , ... ) replacement-list new-line
// # undef identifier new-line
// # line pp-tokens new-line
// # error pp-tokens_opt new-line
// # pragma pp-tokens_opt new-line
// # new-line
const struct production_rule tr_control_line = PR_RULE("control-line", false,
                                                       ALT(CONTROL_LINE_INCLUDE, T_SYM_STR("#"), T_SYM_STR("include"), NT_SYM(tr_pp_tokens), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_DEFINE_OBJECT_LIKE, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(tr_identifier), NT_SYM(tr_replacement_list), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(tr_identifier), NT_SYM(tr_lparen), NT_SYM(tr_identifier_list_opt), T_SYM_STR(")"), NT_SYM(tr_replacement_list), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(tr_identifier), NT_SYM(tr_lparen), T_SYM_STR("..."), T_SYM_STR(")"), NT_SYM(tr_replacement_list), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(tr_identifier), NT_SYM(tr_lparen), NT_SYM(tr_identifier_list), T_SYM_STR(","), T_SYM_STR("..."), T_SYM_STR(")"), NT_SYM(tr_replacement_list), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_UNDEF, T_SYM_STR("#"), T_SYM_STR("undef"), NT_SYM(tr_identifier), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_LINE, T_SYM_STR("#"), T_SYM_STR("line"), NT_SYM(tr_pp_tokens), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_ERROR, T_SYM_STR("#"), T_SYM_STR("error"), NT_SYM(tr_pp_tokens_opt), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_PRAGMA, T_SYM_STR("#"), T_SYM_STR("pragma"), NT_SYM(tr_pp_tokens_opt), T_SYM_STR("\n")),
                                                       ALT(CONTROL_LINE_EMPTY, T_SYM_STR("#"), T_SYM_STR("\n")));

// text-line: pp-tokens_opt new-line
const struct production_rule tr_non_hashtag = PR_RULE("non-hashtag", false, ALT(NO_TAG, T_SYM_FN(match_non_hashtag)));

const struct production_rule tr_tokens_not_starting_with_hashtag = PR_RULE("tokens-not-starting-with-hashtag", true,
                                                                           ALT(LIST_RULE_ONE, NT_SYM(tr_non_hashtag)),
                                                                           ALT(LIST_RULE_MULTI, NT_SYM(tr_tokens_not_starting_with_hashtag), NT_SYM(tr_preprocessing_token)));
const struct production_rule tr_tokens_not_starting_with_hashtag_opt = OPT("tokens-not-starting-with-hashtag_opt", tr_tokens_not_starting_with_hashtag);
const struct production_rule tr_text_line = PR_RULE("text-line", false, ALT(NO_TAG, NT_SYM(tr_tokens_not_starting_with_hashtag_opt), T_SYM_STR("\n")));

// non-directive: pp-tokens new-line
const struct production_rule tr_not_directive_name = PR_RULE("not-directive-name", false, ALT(NO_TAG, T_SYM_FN(match_non_directive_name)));
const struct production_rule tr_tokens_not_starting_with_directive_name = PR_RULE("tokens-not-starting-with-directive-name", true,
                                                                                  ALT(LIST_RULE_ONE, NT_SYM(tr_not_directive_name)),
                                                                                  ALT(LIST_RULE_MULTI, NT_SYM(tr_tokens_not_starting_with_directive_name), NT_SYM(tr_preprocessing_token)));
const struct production_rule tr_non_directive = PR_RULE("non-directive", false, ALT(NO_TAG, NT_SYM(tr_tokens_not_starting_with_directive_name), T_SYM_STR("\n")));

// lparen: a ( character not immediately preceded by white-space
const struct production_rule tr_lparen = PR_RULE("lparen", false, ALT(NO_TAG, T_SYM_FN(match_lparen)));

// replacement-list: pp-tokens_opt
const struct production_rule tr_replacement_list = PR_RULE("replacement-list", false, ALT(NO_TAG, NT_SYM(tr_pp_tokens_opt)));

// pp-tokens: preprocessing-token pp-tokens preprocessing-token
const struct production_rule tr_pp_tokens = PR_RULE("pp-tokens", true,
                                                    ALT(LIST_RULE_ONE, NT_SYM(tr_preprocessing_token)),
                                                    ALT(LIST_RULE_MULTI, NT_SYM(tr_pp_tokens), NT_SYM(tr_preprocessing_token)));

const struct production_rule tr_pp_tokens_opt = OPT("pp-tokens_opt", tr_pp_tokens);

const struct production_rule tr_preprocessing_token = PR_RULE("preprocessing-token", false, ALT(NO_TAG, T_SYM_FN(match_preprocessing_token)));

// identifier-list:
//      identifier
//      identifier-list , identifier
const struct production_rule tr_identifier_list = PR_RULE("identifier-list", true,
                                                          ALT(LIST_RULE_ONE, NT_SYM(tr_identifier)),
                                                          ALT(LIST_RULE_MULTI, NT_SYM(tr_identifier_list), T_SYM_STR(","), NT_SYM(tr_identifier)));
const struct production_rule tr_identifier_list_opt = OPT("identifier-list_opt", tr_identifier_list);

const struct production_rule tr_identifier = PR_RULE("identifier", false, ALT(NO_TAG, T_SYM_FN(match_identifier)));
const struct production_rule tr_identifier_opt = OPT("identifier_opt", tr_identifier);

const struct production_rule tr_constant_expression = PR_RULE("constant-expression", false, ALT(NO_TAG, NT_SYM(tr_conditional_expression)));

const struct production_rule tr_conditional_expression = PR_RULE("conditional-expression", false,
                                                                 ALT(COND_EXPR_LOGICAL_OR, NT_SYM(tr_logical_or_expression)),
                                                                 ALT(COND_EXPR_NORMAL, NT_SYM(tr_logical_or_expression), T_SYM_STR("?"), NT_SYM(tr_expression), T_SYM_STR(":"), NT_SYM(tr_conditional_expression)));

const struct production_rule tr_logical_or_expression = PR_RULE("logical-or-expression", false,
                                                                ALT(LOGICAL_OR_EXPR_LOGICAL_AND, NT_SYM(tr_logical_and_expression)),
                                                                ALT(LOGICAL_OR_EXPR_NORMAL, NT_SYM(tr_logical_or_expression), T_SYM_STR("||"), NT_SYM(tr_logical_and_expression)));

const struct production_rule tr_logical_and_expression = PR_RULE("logical-and-expression", false,
                                                                 ALT(LOGICAL_AND_EXPR_INCLUSIVE_OR, NT_SYM(tr_inclusive_or_expression)),
                                                                 ALT(LOGICAL_AND_EXPR_NORMAL, NT_SYM(tr_logical_and_expression), T_SYM_STR("&&"), NT_SYM(tr_inclusive_or_expression)));

const struct production_rule tr_inclusive_or_expression = PR_RULE("inclusive-or-expression", false,
                                                                  ALT(INCLUSIVE_OR_EXPR_EXCLUSIVE_OR, NT_SYM(tr_exclusive_or_expression)),
                                                                  ALT(INCLUSIVE_OR_EXPR_NORMAL, NT_SYM(tr_inclusive_or_expression), T_SYM_STR("|"), NT_SYM(tr_exclusive_or_expression)));

const struct production_rule tr_exclusive_or_expression = PR_RULE("exclusive-or-expression", false,
                                                                  ALT(EXCLUSIVE_OR_EXPR_AND, NT_SYM(tr_and_expression)),
                                                                  ALT(EXCLUSIVE_OR_EXPR_NORMAL, NT_SYM(tr_exclusive_or_expression), T_SYM_STR("^"), NT_SYM(tr_and_expression)));

const struct production_rule tr_and_expression = PR_RULE("and-expression", false,
                                                         ALT(AND_EXPR_EQUALITY, NT_SYM(tr_equality_expression)),
                                                         ALT(AND_EXPR_NORMAL, NT_SYM(tr_and_expression), T_SYM_STR("&"), NT_SYM(tr_equality_expression)));

const struct production_rule tr_equality_expression = PR_RULE("equality-expression", false,
                                                              ALT(EQUALITY_EXPR_RELATIONAL, NT_SYM(tr_relational_expression)),
                                                              ALT(EQUALITY_EXPR_EQUAL, NT_SYM(tr_equality_expression), T_SYM_STR("=="), NT_SYM(tr_relational_expression)),
                                                              ALT(EQUALITY_EXPR_NOT_EQUAL, NT_SYM(tr_equality_expression), T_SYM_STR("!="), NT_SYM(tr_relational_expression)));

const struct production_rule tr_relational_expression = PR_RULE("relational-expression", false,
                                                                ALT(RELATIONAL_EXPR_SHIFT, NT_SYM(tr_shift_expression)),
                                                                ALT(RELATIONAL_EXPR_LESS, NT_SYM(tr_relational_expression), T_SYM_STR("<"), NT_SYM(tr_shift_expression)),
                                                                ALT(RELATIONAL_EXPR_GREATER, NT_SYM(tr_relational_expression), T_SYM_STR(">"), NT_SYM(tr_shift_expression)),
                                                                ALT(RELATIONAL_EXPR_LEQ, NT_SYM(tr_relational_expression), T_SYM_STR("<="), NT_SYM(tr_shift_expression)),
                                                                ALT(RELATIONAL_EXPR_GEQ, NT_SYM(tr_relational_expression), T_SYM_STR(">="), NT_SYM(tr_shift_expression)));

const struct production_rule tr_shift_expression = PR_RULE("shift-expression", false,
                                                           ALT(SHIFT_EXPR_ADDITIVE, NT_SYM(tr_additive_expression)),
                                                           ALT(SHIFT_EXPR_LEFT, NT_SYM(tr_shift_expression), T_SYM_STR("<<"), NT_SYM(tr_additive_expression)),
                                                           ALT(SHIFT_EXPR_RIGHT, NT_SYM(tr_shift_expression), T_SYM_STR(">>"), NT_SYM(tr_additive_expression)));

const struct production_rule tr_additive_expression = PR_RULE("additive-expression", false,
                                                              ALT(ADDITIVE_EXPR_MULT, NT_SYM(tr_multiplicative_expression)),
                                                              ALT(ADDITIVE_EXPR_PLUS, NT_SYM(tr_additive_expression), T_SYM_STR("+"), NT_SYM(tr_multiplicative_expression)),
                                                              ALT(ADDITIVE_EXPR_MINUS, NT_SYM(tr_additive_expression), T_SYM_STR("-"), NT_SYM(tr_multiplicative_expression)));

const struct production_rule tr_multiplicative_expression = PR_RULE("multiplicative-expression", false,
                                                                    ALT(MULTIPLICATIVE_EXPR_CAST, NT_SYM(tr_cast_expression)),
                                                                    ALT(MULTIPLICATIVE_EXPR_MULT, NT_SYM(tr_multiplicative_expression), T_SYM_STR("*"), NT_SYM(tr_cast_expression)),
                                                                    ALT(MULTIPLICATIVE_EXPR_DIV, NT_SYM(tr_multiplicative_expression), T_SYM_STR("/"), NT_SYM(tr_cast_expression)),
                                                                    ALT(MULTIPLICATIVE_EXPR_MOD, NT_SYM(tr_multiplicative_expression), T_SYM_STR("%"), NT_SYM(tr_cast_expression)));

const struct production_rule tr_cast_expression = PR_RULE("cast-expression", false,
                                                          ALT(CAST_EXPR_UNARY, NT_SYM(tr_unary_expression)),
                                                          ALT(CAST_EXPR_NORMAL, T_SYM_STR("("), NT_SYM(tr_type_name), T_SYM_STR(")"), NT_SYM(tr_cast_expression)));

const struct production_rule tr_unary_expression = PR_RULE("unary-expression", false,
                                                           ALT(UNARY_EXPR_POSTFIX, NT_SYM(tr_postfix_expression)),
                                                           ALT(UNARY_EXPR_INC, T_SYM_STR("++"), NT_SYM(tr_unary_expression)),
                                                           ALT(UNARY_EXPR_DEC, T_SYM_STR("--"), NT_SYM(tr_unary_expression)),
                                                           ALT(UNARY_EXPR_UNARY_OP, NT_SYM(tr_unary_operator), NT_SYM(tr_cast_expression)),
                                                           ALT(UNARY_EXPR_SIZEOF_UNARY, T_SYM_STR("sizeof"),  NT_SYM(tr_unary_expression)),
                                                           ALT(UNARY_EXPR_SIZEOF_TYPE, T_SYM_STR("sizeof"), T_SYM_STR("("), NT_SYM(tr_type_name), T_SYM_STR(")")));

const struct production_rule tr_postfix_expression = PR_RULE("postfix-expression", false,
                                                             ALT(POSTFIX_EXPR_PRIMARY, NT_SYM(tr_primary_expression)),
                                                             ALT(POSTFIX_EXPR_ARRAY_ACCESS, NT_SYM(tr_postfix_expression), T_SYM_STR("["), NT_SYM(tr_expression), T_SYM_STR("]")),
                                                             ALT(POSTFIX_EXPR_FUNC, NT_SYM(tr_postfix_expression), T_SYM_STR("("), NT_SYM(tr_argument_expression_list_opt), T_SYM_STR(")")),
                                                             ALT(POSTFIX_EXPR_DOT, NT_SYM(tr_postfix_expression), T_SYM_STR("."), NT_SYM(tr_identifier)),
                                                             ALT(POSTFIX_EXPR_ARROW, NT_SYM(tr_postfix_expression), T_SYM_STR("->"), NT_SYM(tr_identifier)),
                                                             ALT(POSTFIX_EXPR_INC, NT_SYM(tr_postfix_expression), T_SYM_STR("++")),
                                                             ALT(POSTFIX_EXPR_DEC, NT_SYM(tr_postfix_expression), T_SYM_STR("--")),
                                                             ALT(POSTFIX_EXPR_COMPOUND_LITERAL, T_SYM_STR("("), NT_SYM(tr_type_name), T_SYM_STR(")"), T_SYM_STR("{"), NT_SYM(tr_initializer_list), T_SYM_STR("}")));

const struct production_rule tr_argument_expression_list = PR_RULE("argument-expression-list", true,
                                                                   ALT(LIST_RULE_ONE, NT_SYM(tr_assignment_expression)),
                                                                   ALT(LIST_RULE_MULTI, NT_SYM(tr_argument_expression_list), T_SYM_STR(","), NT_SYM(tr_assignment_expression)));
const struct production_rule tr_argument_expression_list_opt = OPT("argument-expression-list_opt", tr_argument_expression_list);

const struct production_rule tr_unary_operator = PR_RULE("unary-operator", false,
                                                         ALT(UNARY_OPERATOR_PLUS, T_SYM_STR("+")),
                                                         ALT(UNARY_OPERATOR_MINUS, T_SYM_STR("-")),
                                                         ALT(UNARY_OPERATOR_BITWISE_NOT, T_SYM_STR("~")),
                                                         ALT(UNARY_OPERATOR_LOGICAL_NOT, T_SYM_STR("!")),
                                                         ALT(UNARY_OPERATOR_DEREFERENCE, T_SYM_STR("*")),
                                                         ALT(UNARY_OPERATOR_ADDRESS_OF, T_SYM_STR("&")));

const struct production_rule tr_primary_expression = PR_RULE("primary-expression", false,
                                                             ALT(PRIMARY_EXPR_IDENTIFIER, NT_SYM(tr_identifier)),
                                                             ALT(PRIMARY_EXPR_CONSTANT, NT_SYM(tr_constant)),
                                                             ALT(PRIMARY_EXPR_STRING, NT_SYM(tr_string_literal)),
                                                             ALT(PRIMARY_EXPR_PARENS, T_SYM_STR("("), NT_SYM(tr_expression), T_SYM_STR(")")));

const struct production_rule tr_constant = PR_RULE("constant", false,
                                                   ALT(CONSTANT_INTEGER, NT_SYM(tr_integer_constant)),
                                                   ALT(CONSTANT_FLOAT, NT_SYM(tr_floating_constant)),
                                                   ALT(CONSTANT_ENUM, NT_SYM(tr_enumeration_constant)),
                                                   ALT(CONSTANT_CHARACTER, NT_SYM(tr_character_constant)));

const struct production_rule tr_integer_constant = PR_RULE("integer-constant", false, ALT(NO_TAG, T_SYM_FN(match_integer_constant)));
const struct production_rule tr_floating_constant = PR_RULE("floating-constant", false, ALT(NO_TAG, T_SYM_FN(match_floating_constant)));
const struct production_rule tr_enumeration_constant = PR_RULE("enumeration-constant", false, ALT(NO_TAG, NT_SYM(tr_identifier)));
const struct production_rule tr_character_constant = PR_RULE("character-constant", false, ALT(NO_TAG, T_SYM_FN(match_character_constant)));

const struct production_rule tr_expression = PR_RULE("expression", true,
                                                     ALT(LIST_RULE_ONE, NT_SYM(tr_assignment_expression)),
                                                     ALT(LIST_RULE_MULTI, NT_SYM(tr_expression), T_SYM_STR(","), NT_SYM(tr_assignment_expression)));

const struct production_rule tr_assignment_expression = PR_RULE("assignment-expression", false,
                                                                ALT(ASSIGNMENT_EXPR_CONDITIONAL, NT_SYM(tr_conditional_expression)),
                                                                ALT(ASSIGNMENT_EXPR_NORMAL, NT_SYM(tr_unary_expression), NT_SYM(tr_assignment_operator), NT_SYM(tr_assignment_expression)));
const struct production_rule tr_assignment_expression_opt = OPT("assignment_expression_opt", tr_assignment_expression);

const struct production_rule tr_assignment_operator = PR_RULE("assignment-operator", false,
                                                              ALT(ASSIGNMENT_OPERATOR_ASSIGN, T_SYM_STR("=")),
                                                              ALT(ASSIGNMENT_OPERATOR_MULTIPLY_ASSIGN, T_SYM_STR("*=")),
                                                              ALT(ASSIGNMENT_OPERATOR_DIVIDE_ASSIGN, T_SYM_STR("/=")),
                                                              ALT(ASSIGNMENT_OPERATOR_MODULO_ASSIGN, T_SYM_STR("%=")),
                                                              ALT(ASSIGNMENT_OPERATOR_ADD_ASSIGN, T_SYM_STR("+=")),
                                                              ALT(ASSIGNMENT_OPERATOR_SUBTRACT_ASSIGN, T_SYM_STR("-=")),
                                                              ALT(ASSIGNMENT_OPERATOR_SHIFT_LEFT_ASSIGN, T_SYM_STR("<<=")),
                                                              ALT(ASSIGNMENT_OPERATOR_SHIFT_RIGHT_ASSIGN, T_SYM_STR(">>=")),
                                                              ALT(ASSIGNMENT_OPERATOR_BITWISE_AND_ASSIGN, T_SYM_STR("&=")),
                                                              ALT(ASSIGNMENT_OPERATOR_BITWISE_XOR_ASSIGN, T_SYM_STR("^=")),
                                                              ALT(ASSIGNMENT_OPERATOR_BITWISE_OR_ASSIGN, T_SYM_STR("|=")));

const struct production_rule tr_string_literal = PR_RULE("string-literal", false, ALT(NO_TAG, T_SYM_FN(match_string_literal)));

const struct production_rule tr_initializer = PR_RULE("initializer", false,
                                                      ALT(INITIALIZER_ASSIGNMENT, NT_SYM(tr_assignment_expression)),
                                                      ALT(INITIALIZER_BRACES, T_SYM_STR("{"), NT_SYM(tr_initializer_list), T_SYM_STR("}")),
                                                      ALT(INITIALIZER_BRACES_TRAILING_COMMA, T_SYM_STR("{"), NT_SYM(tr_initializer_list), T_SYM_STR(","), T_SYM_STR("}")));

const struct production_rule tr_initializer_list = PR_RULE("initializer-list", true,
                                                           ALT(LIST_RULE_ONE, NT_SYM(tr_designation_opt), NT_SYM(tr_initializer)),
                                                           ALT(LIST_RULE_MULTI, NT_SYM(tr_initializer_list), T_SYM_STR(","), NT_SYM(tr_designation_opt), NT_SYM(tr_initializer)));

const struct production_rule tr_designation = PR_RULE("designation", false, ALT(NO_TAG, NT_SYM(tr_designator_list), T_SYM_STR("=")));
const struct production_rule tr_designation_opt = OPT("designation_opt", tr_designation);

const struct production_rule tr_designator_list = PR_RULE("designator-list", true,
                                                          ALT(LIST_RULE_ONE, NT_SYM(tr_designator)),
                                                          ALT(LIST_RULE_MULTI, NT_SYM(tr_designator_list), NT_SYM(tr_designator)));

const struct production_rule tr_designator = PR_RULE("designator", false,
                                                     ALT(DESIGNATOR_ARRAY, T_SYM_STR("["), NT_SYM(tr_constant_expression), T_SYM_STR("]")),
                                                     ALT(DESIGNATOR_DOT, T_SYM_STR("."), NT_SYM(tr_identifier)));

const struct production_rule tr_type_name = PR_RULE("type-name", false,
                                                    ALT(NO_TAG, NT_SYM(tr_specifier_qualifier_list), NT_SYM(tr_abstract_declarator_opt)));

const struct production_rule tr_specifier_qualifier_list = PR_RULE("specifier-qualifier-list", false,
                                                                   ALT(SPECIFIER_QUALIFIER_LIST_SPECIFIER, NT_SYM(tr_type_specifier), NT_SYM(tr_specifier_qualifier_list_opt)),
                                                                   ALT(SPECIFIER_QUALIFIER_LIST_QUALIFIER, NT_SYM(tr_type_qualifier), NT_SYM(tr_specifier_qualifier_list_opt)));
const struct production_rule tr_specifier_qualifier_list_opt = OPT("specifier-qualifier-list_opt", tr_specifier_qualifier_list);

const struct production_rule tr_type_specifier = PR_RULE("type-specifier", false,
                                                         ALT(TYPE_SPECIFIER_VOID, T_SYM_STR("void")),
                                                         ALT(TYPE_SPECIFIER_CHAR, T_SYM_STR("char")),
                                                         ALT(TYPE_SPECIFIER_SHORT, T_SYM_STR("short")),
                                                         ALT(TYPE_SPECIFIER_INT, T_SYM_STR("int")),
                                                         ALT(TYPE_SPECIFIER_LONG, T_SYM_STR("long")),
                                                         ALT(TYPE_SPECIFIER_FLOAT, T_SYM_STR("float")),
                                                         ALT(TYPE_SPECIFIER_DOUBLE, T_SYM_STR("double")),
                                                         ALT(TYPE_SPECIFIER_SIGNED, T_SYM_STR("signed")),
                                                         ALT(TYPE_SPECIFIER_UNSIGNED, T_SYM_STR("unsigned")),
                                                         ALT(TYPE_SPECIFIER_BOOL, T_SYM_STR("_Bool")),
                                                         ALT(TYPE_SPECIFIER_COMPLEX, T_SYM_STR("_Complex")),
                                                         ALT(TYPE_SPECIFIER_STRUCT_OR_UNION, NT_SYM(tr_struct_or_union_specifier), T_SYM_STR("*")),
                                                         ALT(TYPE_SPECIFIER_ENUM, NT_SYM(tr_enum_specifier)),
                                                         ALT(TYPE_SPECIFIER_TYPEDEF_NAME, NT_SYM(tr_typedef_name)));

const struct production_rule tr_struct_or_union_specifier = PR_RULE("struct-or-union-specifier", false,
                                                                    ALT(STRUCT_OR_UNION_SPECIFIER_DEFINITION, NT_SYM(tr_struct_or_union), NT_SYM(tr_identifier_opt), T_SYM_STR("{"), NT_SYM(tr_struct_declaration_list), T_SYM_STR("}")),
                                                                    ALT(STRUCT_OR_UNION_SPECIFIER_DECLARATION, NT_SYM(tr_struct_or_union), NT_SYM(tr_identifier)));

const struct production_rule tr_struct_or_union = PR_RULE("struct-or-union", false,
                                                          ALT(STRUCT_OR_UNION_STRUCT, T_SYM_STR("struct")),
                                                          ALT(STRUCT_OR_UNION_UNION, T_SYM_STR("union")));

const struct production_rule tr_struct_declaration_list = PR_RULE("struct-declaration-list", true,
                                                                  ALT(LIST_RULE_ONE, NT_SYM(tr_struct_declaration)),
                                                                  ALT(LIST_RULE_MULTI, NT_SYM(tr_struct_declaration_list), NT_SYM(tr_struct_declaration)));

const struct production_rule tr_struct_declaration = PR_RULE("struct-declaration", false,
                                                             ALT(NO_TAG, NT_SYM(tr_specifier_qualifier_list), NT_SYM(tr_struct_declarator_list), T_SYM_STR(";")));

const struct production_rule tr_struct_declarator_list = PR_RULE("struct-declarator-list", true,
                                                                 ALT(LIST_RULE_ONE, NT_SYM(tr_struct_declarator)),
                                                                 ALT(LIST_RULE_MULTI, NT_SYM(tr_struct_declarator_list), T_SYM_STR(","), NT_SYM(tr_struct_declarator)));

const struct production_rule tr_struct_declarator = PR_RULE("struct-declarator", false,
                                                            ALT(STRUCT_DECLARATOR_NORMAL, NT_SYM(tr_declarator)),
                                                            ALT(STRUCT_DECLARATOR_BITFIELD, NT_SYM(tr_declarator_opt), T_SYM_STR(":"), NT_SYM(tr_constant_expression)));

const struct production_rule tr_declarator = PR_RULE("declarator", false, ALT(NO_TAG, NT_SYM(tr_pointer_opt), NT_SYM(tr_direct_declarator)));
const struct production_rule tr_declarator_opt = OPT("declarator_opt", tr_declarator);

const struct production_rule tr_direct_declarator = PR_RULE("direct-declarator", false,
                                                            ALT(DIRECT_DECLARATOR_IDENTIFIER, NT_SYM(tr_identifier)),
                                                            ALT(DIRECT_DECLARATOR_PARENS, T_SYM_STR("("), NT_SYM(tr_declarator), T_SYM_STR(")")),
                                                            ALT(DIRECT_DECLARATOR_ARRAY, NT_SYM(tr_direct_declarator), T_SYM_STR("["), NT_SYM(tr_type_qualifier_list_opt), NT_SYM(tr_assignment_expression_opt), T_SYM_STR("]")),
                                                            ALT(DIRECT_DECLARATOR_ARRAY_STATIC, NT_SYM(tr_direct_declarator), T_SYM_STR("["), T_SYM_STR("static"), NT_SYM(tr_type_qualifier_list_opt), NT_SYM(tr_assignment_expression), T_SYM_STR("]")),
                                                            ALT(DIRECT_DECLARATOR_ARRAY_STATIC_2, NT_SYM(tr_direct_declarator), T_SYM_STR("["), NT_SYM(tr_type_qualifier_list), T_SYM_STR("static"), NT_SYM(tr_assignment_expression), T_SYM_STR("]")),
                                                            ALT(DIRECT_DECLARATOR_ARRAY_ASTERISK, NT_SYM(tr_direct_declarator), T_SYM_STR("["), NT_SYM(tr_type_qualifier_list_opt), T_SYM_STR("*"), T_SYM_STR("]")),
                                                            ALT(DIRECT_DECLARATOR_FUNCTION, NT_SYM(tr_direct_declarator), T_SYM_STR("("), NT_SYM(tr_parameter_type_list), T_SYM_STR(")")),
                                                            ALT(DIRECT_DECLARATOR_FUNCTION_OLD, NT_SYM(tr_direct_declarator), T_SYM_STR("("), NT_SYM(tr_identifier_list_opt), T_SYM_STR(")")));

const struct production_rule tr_enum_specifier = PR_RULE("enum-specifier", false,
                                                         ALT(ENUM_SPECIFIER_DEFINITION, T_SYM_STR("enum"), NT_SYM(tr_identifier_opt), T_SYM_STR("{"), NT_SYM(tr_enumerator_list), T_SYM_STR("}")),
                                                         ALT(ENUM_SPECIFIER_DEFINITION_TRAILING_COMMA, T_SYM_STR("enum"), NT_SYM(tr_identifier_opt), T_SYM_STR("{"), NT_SYM(tr_enumerator_list), T_SYM_STR(","), T_SYM_STR("}")),
                                                         ALT(ENUM_SPECIFIER_DECLARATION, T_SYM_STR("enum"), NT_SYM(tr_identifier)));

const struct production_rule tr_enumerator_list = PR_RULE("enumerator-list", true,
                                                          ALT(LIST_RULE_ONE, NT_SYM(tr_enumerator)),
                                                          ALT(LIST_RULE_MULTI, NT_SYM(tr_enumerator_list), T_SYM_STR(","), NT_SYM(tr_enumerator)));

const struct production_rule tr_enumerator = PR_RULE("enumerator", false,
                                                     ALT(ENUMERATOR_NO_ASSIGNMENT, NT_SYM(tr_enumeration_constant)),
                                                     ALT(ENUMERATOR_ASSIGNMENT, NT_SYM(tr_enumeration_constant), T_SYM_STR("="), NT_SYM(tr_constant_expression)));

const struct production_rule tr_abstract_declarator = PR_RULE("abstract-declarator", false,
                                                              ALT(ABSTRACT_DECLARATOR_POINTER, NT_SYM(tr_pointer)),
                                                              ALT(ABSTRACT_DECLARATOR_DIRECT, NT_SYM(tr_direct_abstract_declarator_opt), NT_SYM(tr_pointer_opt)));
const struct production_rule tr_abstract_declarator_opt = OPT("abstract-declarator_opt", tr_abstract_declarator);

// the C standard writes this as
// pointer:
//  * type-qualifier-list_opt
//  * type-qualifier-list_opt pointer
// and I'm not sure why
const struct production_rule tr_pointer = PR_RULE("pointer", false,
                                                  ALT(NO_TAG, T_SYM_STR("*"), NT_SYM(tr_type_qualifier_list_opt), NT_SYM(tr_pointer_opt)));
const struct production_rule tr_pointer_opt = OPT("pointer_opt", tr_pointer);

const struct production_rule tr_type_qualifier = PR_RULE("type-qualifier", false,
                                                         ALT(TYPE_QUALIFIER_CONST, T_SYM_STR("const")),
                                                         ALT(TYPE_QUALIFIER_RESTRICT, T_SYM_STR("restrict")),
                                                         ALT(TYPE_QUALIFIER_VOLATILE, T_SYM_STR("volatile")));

const struct production_rule tr_type_qualifier_list = PR_RULE("type-qualifier-list", true,
                                                              ALT(LIST_RULE_ONE, NT_SYM(tr_type_qualifier)),
                                                              ALT(LIST_RULE_MULTI, NT_SYM(tr_type_qualifier_list), NT_SYM(tr_type_qualifier)));

const struct production_rule tr_type_qualifier_list_opt = OPT("type-qualifier-list_opt", tr_type_qualifier_list);

const struct production_rule tr_direct_abstract_declarator = PR_RULE("direct-abstract-declarator", false,
                                                                     ALT(DIRECT_ABSTRACT_DECLARATOR_PARENS, NT_SYM(tr_direct_abstract_declarator_opt), T_SYM_STR("("), NT_SYM(tr_parameter_type_list_opt), T_SYM_STR(")")),
                                                                     ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY, NT_SYM(tr_direct_abstract_declarator_opt), T_SYM_STR("["), NT_SYM(tr_type_qualifier_list_opt), NT_SYM(tr_assignment_expression_opt), T_SYM_STR("]")),
                                                                     ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_STATIC, NT_SYM(tr_direct_abstract_declarator_opt), T_SYM_STR("["), T_SYM_STR("static"), NT_SYM(tr_type_qualifier_list_opt), NT_SYM(tr_assignment_expression), T_SYM_STR("]")),
                                                                     ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_QUALIFIER_STATIC, NT_SYM(tr_direct_abstract_declarator_opt), T_SYM_STR("["), NT_SYM(tr_type_qualifier_list), T_SYM_STR("static"), NT_SYM(tr_assignment_expression), T_SYM_STR("]")),
                                                                     ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_ASTERISK, NT_SYM(tr_direct_abstract_declarator_opt), T_SYM_STR("["), T_SYM_STR("*"), T_SYM_STR("]")));
const struct production_rule tr_direct_abstract_declarator_opt = OPT("direct_abstract_declarator_opt", tr_direct_abstract_declarator);

const struct production_rule tr_parameter_type_list = PR_RULE("parameter-type-list", false,
                                                              ALT(PARAMETER_TYPE_LIST_ELLIPSIS, NT_SYM(tr_parameter_list), T_SYM_STR(","), T_SYM_STR("...")),
                                                              ALT(PARAMETER_TYPE_LIST_NO_ELLIPSIS, NT_SYM(tr_parameter_list)));
const struct production_rule tr_parameter_type_list_opt = OPT("parameter-type-list_opt", tr_parameter_type_list);

const struct production_rule tr_parameter_list = PR_RULE("parameter-list", true,
                                                         ALT(LIST_RULE_ONE, NT_SYM(tr_parameter_declaration)),
                                                         ALT(LIST_RULE_MULTI, NT_SYM(tr_parameter_list), T_SYM_STR(","), NT_SYM(tr_parameter_declaration)));

const struct production_rule tr_parameter_declaration = PR_RULE("parameter-declaration", false,
                                                                ALT(PARAMETER_DECLARATION_DECLARATOR, NT_SYM(tr_declaration_specifiers), NT_SYM(tr_declarator)),
                                                                ALT(PARAMETER_DECLARATION_ABSTRACT, NT_SYM(tr_declaration_specifiers), NT_SYM(tr_abstract_declarator_opt)));

const struct production_rule tr_declaration_specifiers = PR_RULE("declaration-specifiers", false,
                                                                 ALT(DECLARATION_SPECIFIERS_STORAGE_CLASS, NT_SYM(tr_storage_class_specifier), NT_SYM(tr_declaration_specifiers_opt)),
                                                                 ALT(DECLARATION_SPECIFIERS_TYPE_SPECIFIER, NT_SYM(tr_type_specifier), NT_SYM(tr_declaration_specifiers_opt)),
                                                                 ALT(DECLARATION_SPECIFIERS_TYPE_QUALIFIER, NT_SYM(tr_type_qualifier), NT_SYM(tr_declaration_specifiers_opt)),
                                                                 ALT(DECLARATION_SPECIFIERS_FUNCTION_SPECIFIER, NT_SYM(tr_function_specifier), NT_SYM(tr_declaration_specifiers_opt)));
const struct production_rule tr_declaration_specifiers_opt = OPT("declaration-specifiers_opt", tr_declaration_specifiers);

const struct production_rule tr_storage_class_specifier = PR_RULE("storage-class-specifier", false,
                                                                  ALT(STORAGE_CLASS_SPECIFIER_TYPEDEF, T_SYM_STR("typedef")),
                                                                  ALT(STORAGE_CLASS_SPECIFIER_EXTERN, T_SYM_STR("extern")),
                                                                  ALT(STORAGE_CLASS_SPECIFIER_STATIC, T_SYM_STR("static")),
                                                                  ALT(STORAGE_CLASS_SPECIFIER_AUTO, T_SYM_STR("auto")),
                                                                  ALT(STORAGE_CLASS_SPECIFIER_REGISTER, T_SYM_STR("register")));

const struct production_rule tr_function_specifier = PR_RULE("function-specifier", false, ALT(NO_TAG, T_SYM_STR("inline")));

const struct production_rule tr_typedef_name = PR_RULE("typedef-name", false, ALT(NO_TAG, NT_SYM(tr_identifier)));
