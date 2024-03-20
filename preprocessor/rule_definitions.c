#include <stdbool.h>
#include "preprocessor/parser.h"

#define ALT(_tag, ...)                                                   \
    ((const struct alternative) {                                        \
        .symbols=(struct symbol[]) {__VA_ARGS__},                        \
        .n=sizeof((struct symbol[]){__VA_ARGS__})/sizeof(struct symbol), \
        .tag=_tag                                                        \
    })

#define PR_RULE(_name, ...)                                                       \
    ((const struct production_rule) {                                             \
        .name=_name,                                                              \
        .alternatives=(const struct alternative[]) {__VA_ARGS__},                 \
        .n=sizeof((struct alternative[]){__VA_ARGS__})/sizeof(struct alternative) \
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

#define OPT(_name, _rule) PR_RULE(_name, ALT(OPT_ONE, NT_SYM(_rule)), EMPTY_ALT(OPT_NONE))

static bool match_preprocessing_token(__attribute__((unused)) struct preprocessing_token token) {
    return !token_is_str(token, (unsigned char*)"\n");
}

static bool match_lparen(struct preprocessing_token token) {
    return token_is_str(token, (unsigned char*)"(") && !token.after_whitespace;
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
    return match_preprocessing_token(token) && !token_is_str(token, (unsigned char*)"#");
}

static bool match_non_directive_name(struct preprocessing_token token) {
    return !token_is_str(token, (unsigned char*)"define") &&
           !token_is_str(token, (unsigned char*)"undef") &&
           !token_is_str(token, (unsigned char*)"if") &&
           !token_is_str(token, (unsigned char*)"ifdef") &&
           !token_is_str(token, (unsigned char*)"ifndef") &&
           !token_is_str(token, (unsigned char*)"elif") &&
           !token_is_str(token, (unsigned char*)"else") &&
           !token_is_str(token, (unsigned char*)"endif") &&
           !token_is_str(token, (unsigned char*)"include") &&
           !token_is_str(token, (unsigned char*)"line") &&
           !token_is_str(token, (unsigned char*)"error") &&
           !token_is_str(token, (unsigned char*)"pragma");
}

static bool is_int_suffix(struct str_view str_view) {
#define IS(str) str_view_cstr_eq(str_view, (const unsigned char*)str)
    return IS("u") || IS("U") || IS("l") || IS("L")
            || IS("ll") || IS("LL")
            || IS("ul") || IS("uL") || IS("Ul") || IS("UL")
            || IS("lu") || IS("lU") || IS("Lu") || IS("LU")
            || IS("llu") || IS("llU") || IS("lLu") || IS("LLu") || IS("LLU")
            || IS("ull") || IS("uLL") || IS("Ull") || IS("ULL");
#undef IS
}

static bool match_integer_constant(struct preprocessing_token token) {
    if (token.name.n == 0) return false;
    else if (token.name.n == 1) return token.name.first[0] >= '0' && token.name.first[0] <= '9';
    else {
        size_t i;
        if (token.name.first[0] == '0' && (token.name.first[1] == 'x' || token.name.first[1] == 'X')) {
            for (i = 2; i < token.name.n && isxdigit(token.name.first[i]); i++);
            if (i == 2) return false;
        } else if (token.name.first[0] == '0') {
            for (i = 1; i < token.name.n && token.name.first[i] >= '0' && token.name.first[i] <= '7'; i++);
        } else if (isdigit(token.name.first[0])) {
            for (i = 1; i < token.name.n && token.name.first[i] >= '0' && token.name.first[i] <= '9'; i++);
        } else {
            return false;
        }
        return i == token.name.n || is_int_suffix((struct str_view) { .first = &token.name.first[i], .n = token.name.n - i });
    }
}

static ssize_t digit_sequence_helper(struct str_view str_view, size_t i) {
    if (i >= str_view.n || !isdigit(str_view.first[i])) return -1;
    for (; i < str_view.n && isdigit(str_view.first[i]); i++);
    return (ssize_t) i;
}

static ssize_t fractional_constant_helper(struct str_view str_view, size_t i) {
    // shortest possible fractional constant is .0 or 0.
    if (i > str_view.n-2) return -1;
    if (str_view.first[i] == '.') {
        i++;
        // 1 or more digits must follow the dot
        if (i >= str_view.n || !isdigit(str_view.first[i])) return -1;
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.first[i]); i++);
        return (ssize_t) i;
    } else if (isdigit(str_view.first[i])) {
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.first[i]); i++);
        // if we reach the end first, then there's no dot
        if (i >= str_view.n || str_view.first[i] != '.') return -1;
        // scan past the dot
        i++;
        // scan until we reach a non-digit or the end
        for (; i < str_view.n && isdigit(str_view.first[i]); i++);
        return (ssize_t) i;
    } else {
        return -1;
    }
}

static ssize_t exponent_part_helper(struct str_view str_view, size_t i) {
    // shortest possible exponent part is e0 or E0
    if (i > str_view.n-2 || (str_view.first[i] != 'e' && str_view.first[i] != 'E')) return -1;
    i++;
    if (i >= str_view.n) return -1;
    // optional sign
    if (str_view.first[i] == '+' || str_view.first[i] == '-') i++;
    // digit sequence required at the end
    if (i >= str_view.n || !isdigit(str_view.first[i])) return -1;
    // scan until we reach a non-digit or the end
    for (; i < str_view.n && isdigit(str_view.first[i]); i++);
    return (ssize_t) i;
}

static bool is_floating_suffix(unsigned char c) {
    return c == 'f' || c == 'F' || c == 'l' || c == 'L';
}

static bool match_decimal_floating_constant(struct preprocessing_token token) {
    ssize_t fi = fractional_constant_helper(token.name, 0);
    ssize_t di = digit_sequence_helper(token.name, 0);
    if (fi == -1 && di == -1) return false;
    else if (fi != -1) { // fractional-constant exponent-part_opt floating-suffix_opt
        ssize_t ei = exponent_part_helper(token.name, (size_t)fi);
        if (ei == -1) { // exponent part doesn't exist
            // should be a floating suffix or the end
            return (size_t)fi == token.name.n || ((size_t)fi+1 == token.name.n && is_floating_suffix(token.name.first[fi]));
        } else { // exponent part does exist
            // should be a floating suffix or the end
            return (size_t)ei == token.name.n || ((size_t)ei+1 == token.name.n && is_floating_suffix(token.name.first[ei]));
        }
    } else { // digit-sequence exponent-part floating-suffix_opt
        ssize_t ei = exponent_part_helper(token.name, (size_t)di);
        if (ei == -1) return false;
        return (size_t) ei == token.name.n || ((size_t)ei+1 == token.name.n && is_floating_suffix(token.name.first[ei]));
    }
}

bool match_integer_constant_wrapper(const char *str) {
    struct str_view str_view = { .first = (const unsigned char*)str, .n = strlen(str) };
    return match_integer_constant((struct preprocessing_token) { .name = str_view });
}

static bool match_floating_constant(struct preprocessing_token token) {
    // TODO add hex float support
    return match_decimal_floating_constant(token);
}

// preprocessing-file: group_opt
const struct production_rule preprocessing_file = PR_RULE("preprocessing-file", ALT(NO_TAG, NT_SYM(group_opt)));

// group: group-part group group-part
const struct production_rule group = PR_RULE("group",
                                             ALT(LIST_RULE_ONE, NT_SYM(group_part)),
                                             ALT(LIST_RULE_MULTI, NT_SYM(group), NT_SYM(group_part)));
const struct production_rule group_opt = OPT("group_opt", group);

// group-part: if-section control-line text-line # non-directive
const struct production_rule group_part = PR_RULE("group-part",
                                                  ALT(GROUP_PART_IF, NT_SYM(if_section)),
                                                  ALT(GROUP_PART_CONTROL, NT_SYM(control_line)),
                                                  ALT(GROUP_PART_TEXT, NT_SYM(text_line)),
                                                  ALT(GROUP_PART_NON_DIRECTIVE, T_SYM_STR("#"), NT_SYM(non_directive)));

// if-section: if-group elif-groups_opt else-group_opt endif-line
const struct production_rule if_section = PR_RULE("if-section",
                                                  ALT(NO_TAG, NT_SYM(if_group), NT_SYM(elif_groups_opt), NT_SYM(else_group_opt), NT_SYM(endif_line)));

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
const struct production_rule if_group = PR_RULE("if-group",
                                                ALT(IF_GROUP_IF, T_SYM_STR("#"), T_SYM_STR("if"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                ALT(IF_GROUP_IFDEF, T_SYM_STR("#"), T_SYM_STR("ifdef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                ALT(IF_GROUP_IFNDEF, T_SYM_STR("#"), T_SYM_STR("ifndef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)));

// elif-groups: elif-group
//              elif-groups elif-group
const struct production_rule elif_groups = PR_RULE("elif_groups",
                                                    ALT(LIST_RULE_ONE, NT_SYM(elif_group)),
                                                    ALT(LIST_RULE_MULTI, NT_SYM(elif_groups), NT_SYM(elif_group)));
const struct production_rule elif_groups_opt = OPT("elif-groups_opt", elif_groups);

// elif-group: # elif constant-expression new-line group_opt
const struct production_rule elif_group = PR_RULE("elif_group",
                                                   ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("elif"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)));

const struct production_rule else_group = PR_RULE("else-group",
                                                  ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("else"), T_SYM_STR("\n"), NT_SYM(group_opt)));
const struct production_rule else_group_opt = OPT("else-group_opt", else_group);

// endif-line: # endif new-line
const struct production_rule endif_line = PR_RULE("endif-line",
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
const struct production_rule control_line = PR_RULE("control-line",
                                                    ALT(CONTROL_LINE_INCLUDE, T_SYM_STR("#"), T_SYM_STR("include"), NT_SYM(pp_tokens), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_DEFINE_OBJECT_LIKE, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(identifier), NT_SYM(replacement_list), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(identifier), NT_SYM(lparen), NT_SYM(identifier_list_opt), T_SYM_STR(")"), NT_SYM(replacement_list), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(identifier), NT_SYM(lparen), T_SYM_STR("..."), T_SYM_STR(")"), NT_SYM(replacement_list), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS, T_SYM_STR("#"), T_SYM_STR("define"), NT_SYM(identifier), NT_SYM(lparen), NT_SYM(identifier_list), T_SYM_STR(","), T_SYM_STR("..."), T_SYM_STR(")"), NT_SYM(replacement_list), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_UNDEF, T_SYM_STR("#"), T_SYM_STR("undef"), NT_SYM(identifier), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_LINE, T_SYM_STR("#"), T_SYM_STR("line"), NT_SYM(pp_tokens), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_ERROR, T_SYM_STR("#"), T_SYM_STR("error"), NT_SYM(pp_tokens_opt), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_PRAGMA, T_SYM_STR("#"), T_SYM_STR("pragma"), NT_SYM(pp_tokens_opt), T_SYM_STR("\n")),
                                                    ALT(CONTROL_LINE_EMPTY, T_SYM_STR("#"), T_SYM_STR("\n")));

// text-line: pp-tokens_opt new-line
const struct production_rule non_hashtag = PR_RULE("non-hashtag", ALT(NO_TAG, T_SYM_FN(match_non_hashtag)));

const struct production_rule tokens_not_starting_with_hashtag = PR_RULE("tokens-not-starting-with-hashtag",
                                                                        ALT(LIST_RULE_ONE, NT_SYM(non_hashtag)),
                                                                        ALT(LIST_RULE_MULTI, NT_SYM(tokens_not_starting_with_hashtag), NT_SYM(rule_preprocessing_token)));
const struct production_rule tokens_not_starting_with_hashtag_opt = OPT("tokens-not-starting-with-hashtag_opt", tokens_not_starting_with_hashtag);
const struct production_rule text_line = PR_RULE("text-line", ALT(NO_TAG, NT_SYM(tokens_not_starting_with_hashtag_opt), T_SYM_STR("\n")));

// non-directive: pp-tokens new-line
const struct production_rule not_directive_name = PR_RULE("not-directive-name", ALT(NO_TAG, T_SYM_FN(match_non_directive_name)));
const struct production_rule tokens_not_starting_with_directive_name = PR_RULE("tokens-not-starting-with-directive-name",
                                                                               ALT(LIST_RULE_ONE, NT_SYM(not_directive_name)),
                                                                               ALT(LIST_RULE_MULTI, NT_SYM(tokens_not_starting_with_directive_name), NT_SYM(rule_preprocessing_token)));
const struct production_rule non_directive = PR_RULE("non-directive",
                                                     ALT(NO_TAG, NT_SYM(tokens_not_starting_with_directive_name), T_SYM_STR("\n")));

// lparen: a ( character not immediately preceded by white-space
const struct production_rule lparen = PR_RULE("lparen", ALT(NO_TAG, T_SYM_FN(match_lparen)));

// replacement-list: pp-tokens_opt
const struct production_rule replacement_list = PR_RULE("replacement-list", ALT(NO_TAG, NT_SYM(pp_tokens_opt)));

// pp-tokens: preprocessing-token pp-tokens preprocessing-token
const struct production_rule pp_tokens = PR_RULE("pp-tokens",
                                                 ALT(LIST_RULE_ONE, NT_SYM(rule_preprocessing_token)),
                                                 ALT(LIST_RULE_MULTI, NT_SYM(pp_tokens), NT_SYM(rule_preprocessing_token)));

const struct production_rule pp_tokens_opt = OPT("pp-tokens_opt", pp_tokens);

const struct production_rule rule_preprocessing_token = PR_RULE("preprocessing-token", ALT(NO_TAG, T_SYM_FN(match_preprocessing_token)));

// identifier-list:
//      identifier
//      identifier-list , identifier
const struct production_rule identifier_list = PR_RULE("identifier-list",
                                                       ALT(LIST_RULE_ONE, NT_SYM(identifier)),
                                                       ALT(LIST_RULE_MULTI, NT_SYM(identifier_list), T_SYM_STR(","), NT_SYM(identifier)));
const struct production_rule identifier_list_opt = OPT("identifier-list_opt", identifier_list);

const struct production_rule identifier = PR_RULE("identifier", ALT(NO_TAG, T_SYM_FN(match_identifier)));
const struct production_rule identifier_opt = OPT("identifier_opt", identifier);

const struct production_rule constant_expression = PR_RULE("constant-expression", ALT(NO_TAG, NT_SYM(conditional_expression)));

const struct production_rule conditional_expression = PR_RULE("conditional-expression",
        ALT(COND_EXPR_LOGICAL_OR, NT_SYM(logical_or_expression)),
        ALT(COND_EXPR_NORMAL, NT_SYM(logical_or_expression), T_SYM_STR("?"), NT_SYM(expression), T_SYM_STR(":"), NT_SYM(conditional_expression)));

const struct production_rule logical_or_expression = PR_RULE("logical-or-expression",
        ALT(LOGICAL_OR_EXPR_LOGICAL_AND, NT_SYM(logical_and_expression)),
        ALT(LOGICAL_OR_EXPR_NORMAL, NT_SYM(logical_or_expression), T_SYM_STR("||"), NT_SYM(logical_and_expression)));

const struct production_rule logical_and_expression = PR_RULE("logical-and-expression",
        ALT(LOGICAL_AND_EXPR_INCLUSIVE_OR, NT_SYM(inclusive_or_expression)),
        ALT(LOGICAL_AND_EXPR_NORMAL, NT_SYM(logical_and_expression), T_SYM_STR("&&"), NT_SYM(inclusive_or_expression)));

const struct production_rule inclusive_or_expression = PR_RULE("inclusive-or-expression",
        ALT(INCLUSIVE_OR_EXPR_EXCLUSIVE_OR, NT_SYM(exclusive_or_expression)),
        ALT(INCLUSIVE_OR_EXPR_NORMAL, NT_SYM(inclusive_or_expression), T_SYM_STR("|"), NT_SYM(exclusive_or_expression)));

const struct production_rule exclusive_or_expression = PR_RULE("exclusive-or-expression",
        ALT(EXCLUSIVE_OR_EXPR_AND, NT_SYM(and_expression)),
        ALT(EXCLUSIVE_OR_EXPR_NORMAL, NT_SYM(exclusive_or_expression), T_SYM_STR("^"), NT_SYM(and_expression)));

const struct production_rule and_expression = PR_RULE("and-expression",
        ALT(AND_EXPR_EQUALITY, NT_SYM(equality_expression)),
        ALT(AND_EXPR_NORMAL, NT_SYM(and_expression), T_SYM_STR("&"), NT_SYM(equality_expression)));

const struct production_rule equality_expression = PR_RULE("equality-expression",
        ALT(EQUALITY_EXPR_RELATIONAL, NT_SYM(relational_expression)),
        ALT(EQUALITY_EXPR_EQUAL, NT_SYM(equality_expression), T_SYM_STR("=="), NT_SYM(relational_expression)),
        ALT(EQUALITY_EXPR_NOT_EQUAL, NT_SYM(equality_expression), T_SYM_STR("!="), NT_SYM(relational_expression)));

const struct production_rule relational_expression = PR_RULE("relational-expression",
        ALT(RELATIONAL_EXPR_SHIFT, NT_SYM(shift_expression)),
        ALT(RELATIONAL_EXPR_LESS, NT_SYM(relational_expression), T_SYM_STR("<"), NT_SYM(shift_expression)),
        ALT(RELATIONAL_EXPR_GREATER, NT_SYM(relational_expression), T_SYM_STR(">"), NT_SYM(shift_expression)),
        ALT(RELATIONAL_EXPR_LEQ, NT_SYM(relational_expression), T_SYM_STR("<="), NT_SYM(shift_expression)),
        ALT(RELATIONAL_EXPR_GEQ, NT_SYM(relational_expression), T_SYM_STR(">="), NT_SYM(shift_expression)));

const struct production_rule shift_expression = PR_RULE("shift-expression",
        ALT(SHIFT_EXPR_ADDITIVE, NT_SYM(additive_expression)),
        ALT(SHIFT_EXPR_LEFT, NT_SYM(shift_expression), T_SYM_STR("<<"), NT_SYM(additive_expression)),
        ALT(SHIFT_EXPR_RIGHT, NT_SYM(shift_expression), T_SYM_STR(">>"), NT_SYM(additive_expression)));

const struct production_rule additive_expression = PR_RULE("additive-expression",
        ALT(ADDITIVE_EXPR_MULT, NT_SYM(multiplicative_expression)),
        ALT(ADDITIVE_EXPR_PLUS, NT_SYM(additive_expression), T_SYM_STR("+"), NT_SYM(multiplicative_expression)),
        ALT(ADDITIVE_EXPR_MINUS, NT_SYM(additive_expression), T_SYM_STR("-"), NT_SYM(multiplicative_expression)));

const struct production_rule multiplicative_expression = PR_RULE("multiplicative-expression",
        ALT(MULTIPLICATIVE_EXPR_CAST, NT_SYM(cast_expression)),
        ALT(MULTIPLICATIVE_EXPR_MULT, NT_SYM(multiplicative_expression), T_SYM_STR("*"), NT_SYM(cast_expression)),
        ALT(MULTIPLICATIVE_EXPR_DIV, NT_SYM(multiplicative_expression), T_SYM_STR("/"), NT_SYM(cast_expression)),
        ALT(MULTIPLICATIVE_EXPR_MOD, NT_SYM(multiplicative_expression), T_SYM_STR("%"), NT_SYM(cast_expression)));

const struct production_rule cast_expression = PR_RULE("cast-expression",
        ALT(CAST_EXPR_UNARY, NT_SYM(unary_expression)),
        ALT(CAST_EXPR_NORMAL, T_SYM_STR("("), NT_SYM(type_name), T_SYM_STR(")"), NT_SYM(cast_expression)));

const struct production_rule unary_expression = PR_RULE("unary-expression",
        ALT(UNARY_EXPR_POSTFIX, NT_SYM(postfix_expression)),
        ALT(UNARY_EXPR_INC, T_SYM_STR("++"), NT_SYM(unary_expression)),
        ALT(UNARY_EXPR_DEC, T_SYM_STR("--"), NT_SYM(unary_expression)),
        ALT(UNARY_EXPR_UNARY_OP, NT_SYM(unary_operator), NT_SYM(cast_expression)),
        ALT(UNARY_EXPR_SIZEOF_UNARY, T_SYM_STR("sizeof"), T_SYM_STR("("), NT_SYM(unary_expression), T_SYM_STR(")")),
        ALT(UNARY_EXPR_SIZEOF_TYPE, T_SYM_STR("sizeof"), T_SYM_STR("("), NT_SYM(type_name), T_SYM_STR(")")));

const struct production_rule postfix_expression = PR_RULE("postfix-expression",
        ALT(POSTFIX_EXPR_PRIMARY, NT_SYM(primary_expression)),
        ALT(POSTFIX_EXPR_ARRAY_ACCESS, NT_SYM(postfix_expression), T_SYM_STR("["), NT_SYM(expression), T_SYM_STR("]")),
        ALT(POSTFIX_EXPR_FUNC, NT_SYM(postfix_expression), T_SYM_STR("("), NT_SYM(argument_expression_list_opt), T_SYM_STR(")")),
        ALT(POSTFIX_EXPR_DOT, NT_SYM(postfix_expression), T_SYM_STR("."), NT_SYM(identifier)),
        ALT(POSTFIX_EXPR_ARROW, NT_SYM(postfix_expression), T_SYM_STR("->"), NT_SYM(identifier)),
        ALT(POSTFIX_EXPR_INC, NT_SYM(postfix_expression), T_SYM_STR("++")),
        ALT(POSTFIX_EXPR_DEC, NT_SYM(postfix_expression), T_SYM_STR("--")),
        ALT(POSTFIX_EXPR_TYPE_NAME, T_SYM_STR("("), NT_SYM(type_name), T_SYM_STR(")"), T_SYM_STR("{"), NT_SYM(initializer_list), T_SYM_STR("}")));

const struct production_rule argument_expression_list = PR_RULE("argument-expression-list",
        ALT(LIST_RULE_ONE, NT_SYM(assignment_expression)),
        ALT(LIST_RULE_MULTI, NT_SYM(argument_expression_list), T_SYM_STR(","), NT_SYM(assignment_expression)));
const struct production_rule argument_expression_list_opt = OPT("argument-expression-list_opt", argument_expression_list);

const struct production_rule unary_operator = PR_RULE("unary-operator",
                                                      ALT(UNARY_OPERATOR_PLUS, T_SYM_STR("+")),
                                                      ALT(UNARY_OPERATOR_MINUS, T_SYM_STR("-")),
                                                      ALT(UNARY_OPERATOR_BITWISE_NOT, T_SYM_STR("~")),
                                                      ALT(UNARY_OPERATOR_LOGICAL_NOT, T_SYM_STR("!")),
                                                      ALT(UNARY_OPERATOR_DEREFERENCE, T_SYM_STR("*")),
                                                      ALT(UNARY_OPERATOR_ADDRESS_OF, T_SYM_STR("&")));

const struct production_rule primary_expression = PR_RULE("primary-expression",
        ALT(PRIMARY_EXPR_IDENTIFIER, NT_SYM(identifier)),
        ALT(PRIMARY_EXPR_CONSTANT, NT_SYM(constant)),
        ALT(PRIMARY_EXPR_STRING, NT_SYM(string_literal)),
        ALT(PRIMARY_EXPR_PARENS, T_SYM_STR("("), NT_SYM(expression), T_SYM_STR(")")));

const struct production_rule constant = PR_RULE("constant",
        ALT(CONSTANT_INTEGER, NT_SYM(integer_constant)),
        ALT(CONSTANT_FLOAT, NT_SYM(floating_constant)),
        ALT(CONSTANT_ENUM, NT_SYM(enumeration_constant)),
        ALT(CONSTANT_CHARACTER, NT_SYM(character_constant)));

const struct production_rule integer_constant = PR_RULE("integer-constant", ALT(NO_TAG, T_SYM_FN(match_integer_constant)));
const struct production_rule floating_constant = PR_RULE("floating-constant", ALT(NO_TAG, T_SYM_FN(match_floating_constant)));
const struct production_rule enumeration_constant = PR_RULE("enumeration-constant", ALT(NO_TAG, NT_SYM(identifier)));
const struct production_rule character_constant = PR_RULE("character-constant", ALT(NO_TAG, T_SYM_FN(match_character_constant)));

const struct production_rule expression = PR_RULE("expression",
        ALT(LIST_RULE_ONE, NT_SYM(assignment_expression)),
        ALT(LIST_RULE_MULTI, NT_SYM(expression), T_SYM_STR(","), NT_SYM(assignment_expression)));

const struct production_rule assignment_expression = PR_RULE("assignment-expression",
        ALT(ASSIGNMENT_EXPR_CONDITIONAL, NT_SYM(conditional_expression)),
        ALT(ASSIGNMENT_EXPR_NORMAL, NT_SYM(unary_expression), NT_SYM(assignment_operator), NT_SYM(assignment_expression)));
const struct production_rule assignment_expression_opt = OPT("assignment_expression_opt", assignment_expression);

const struct production_rule assignment_operator = PR_RULE("assignment-operator",
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

const struct production_rule string_literal = PR_RULE("string-literal", ALT(NO_TAG, T_SYM_FN(match_string_literal)));

const struct production_rule initializer = PR_RULE("initializer",
        ALT(INITIALIZER_ASSIGNMENT, NT_SYM(assignment_expression)),
        ALT(INITIALIZER_BRACES, T_SYM_STR("{"), NT_SYM(initializer_list), T_SYM_STR("}")),
        ALT(INITIALIZER_BRACES_TRAILING_COMMA, T_SYM_STR("{"), NT_SYM(initializer_list), T_SYM_STR(","), T_SYM_STR("}")));

const struct production_rule initializer_list = PR_RULE("initializer-list",
        ALT(LIST_RULE_ONE, NT_SYM(designation_opt), NT_SYM(initializer)),
        ALT(LIST_RULE_MULTI, NT_SYM(initializer_list), T_SYM_STR(","), NT_SYM(designation_opt), NT_SYM(initializer)));

const struct production_rule designation = PR_RULE("designation", ALT(NO_TAG, NT_SYM(designator_list), T_SYM_STR("=")));
const struct production_rule designation_opt = OPT("designation_opt", designation);

const struct production_rule designator_list = PR_RULE("designator-list",
        ALT(LIST_RULE_ONE, NT_SYM(designator)),
        ALT(LIST_RULE_MULTI, NT_SYM(designator_list), NT_SYM(designator)));

const struct production_rule designator = PR_RULE("designator",
        ALT(DESIGNATOR_ARRAY, T_SYM_STR("["), NT_SYM(constant_expression), T_SYM_STR("]")),
        ALT(DESIGNATOR_DOT, T_SYM_STR("."), NT_SYM(identifier)));

const struct production_rule type_name = PR_RULE("type-name",
                                                 ALT(NO_TAG, NT_SYM(specifier_qualifier_list), NT_SYM(abstract_declarator_opt)));

const struct production_rule specifier_qualifier_list = PR_RULE("specifier-qualifier-list",
                               ALT(SPECIFIER_QUALIFIER_LIST_SPECIFIER, NT_SYM(type_specifier), NT_SYM(specifier_qualifier_list_opt)),
                               ALT(SPECIFIER_QUALIFIER_LIST_QUALIFIER, NT_SYM(type_qualifier), NT_SYM(specifier_qualifier_list_opt)));
const struct production_rule specifier_qualifier_list_opt = OPT("specifier-qualifier-list_opt", specifier_qualifier_list);

const struct production_rule type_specifier = PR_RULE("type-specifier",
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
                                                      ALT(TYPE_SPECIFIER_STRUCT_OR_UNION, NT_SYM(struct_or_union_specifier), T_SYM_STR("*")),
                                                      ALT(TYPE_SPECIFIER_ENUM, NT_SYM(enum_specifier)),
                                                      ALT(TYPE_SPECIFIER_TYPEDEF_NAME, NT_SYM(typedef_name)));

const struct production_rule struct_or_union_specifier = PR_RULE("struct-or-union-specifier",
        ALT(STRUCT_OR_UNION_SPECIFIER_DEFINITION, NT_SYM(struct_or_union), NT_SYM(identifier_opt), T_SYM_STR("{"), NT_SYM(struct_declaration_list), T_SYM_STR("}")),
        ALT(STRUCT_OR_UNION_SPECIFIER_DECLARATION, NT_SYM(struct_or_union), NT_SYM(identifier)));

const struct production_rule struct_or_union = PR_RULE("struct-or-union",
        ALT(STRUCT_OR_UNION_STRUCT, T_SYM_STR("struct")),
        ALT(STRUCT_OR_UNION_UNION, T_SYM_STR("union")));

const struct production_rule struct_declaration_list = PR_RULE("struct-declaration-list",
        ALT(LIST_RULE_ONE, NT_SYM(struct_declaration)),
        ALT(LIST_RULE_MULTI, NT_SYM(struct_declaration_list), NT_SYM(struct_declaration)));

const struct production_rule struct_declaration = PR_RULE("struct-declaration",
        ALT(NO_TAG, NT_SYM(specifier_qualifier_list), NT_SYM(struct_declarator_list), T_SYM_STR(";")));

const struct production_rule struct_declarator_list = PR_RULE("struct-declarator-list",
        ALT(LIST_RULE_ONE, NT_SYM(struct_declarator)),
        ALT(LIST_RULE_MULTI, NT_SYM(struct_declarator_list), T_SYM_STR(","), NT_SYM(struct_declarator)));

const struct production_rule struct_declarator = PR_RULE("struct-declarator",
    ALT(STRUCT_DECLARATOR_NORMAL, NT_SYM(declarator)),
    ALT(STRUCT_DECLARATOR_BITFIELD, NT_SYM(declarator_opt), T_SYM_STR(":"), NT_SYM(constant_expression)));

const struct production_rule declarator = PR_RULE("declarator", ALT(NO_TAG, NT_SYM(pointer_opt), NT_SYM(direct_declarator)));
const struct production_rule declarator_opt = OPT("declarator_opt", declarator);

const struct production_rule direct_declarator = PR_RULE("direct-declarator",
        ALT(DIRECT_DECLARATOR_IDENTIFIER, NT_SYM(identifier)),
        ALT(DIRECT_DECLARATOR_PARENS, T_SYM_STR("("), NT_SYM(declarator), T_SYM_STR(")")),
        ALT(DIRECT_DECLARATOR_ARRAY, NT_SYM(direct_declarator), T_SYM_STR("["), NT_SYM(type_qualifier_list_opt), NT_SYM(assignment_expression_opt), T_SYM_STR("]")),
        ALT(DIRECT_DECLARATOR_ARRAY_STATIC, NT_SYM(direct_declarator), T_SYM_STR("["), T_SYM_STR("static"), NT_SYM(type_qualifier_list_opt), NT_SYM(assignment_expression), T_SYM_STR("]")),
        ALT(DIRECT_DECLARATOR_ARRAY_STATIC_2, NT_SYM(direct_declarator), T_SYM_STR("["), NT_SYM(type_qualifier_list), T_SYM_STR("static"), NT_SYM(assignment_expression), T_SYM_STR("]")),
        ALT(DIRECT_DECLARATOR_ARRAY_ASTERISK, NT_SYM(direct_declarator), T_SYM_STR("["), NT_SYM(type_qualifier_list_opt), T_SYM_STR("*"), T_SYM_STR("]")),
        ALT(DIRECT_DECLARATOR_FUNCTION, NT_SYM(direct_declarator), T_SYM_STR("("), NT_SYM(parameter_type_list), T_SYM_STR(")")),
        ALT(DIRECT_DECLARATOR_FUNCTION_OLD, NT_SYM(direct_declarator), T_SYM_STR("("), NT_SYM(identifier_list_opt), T_SYM_STR(")")));

const struct production_rule enum_specifier = PR_RULE("enum-specifier",
        ALT(ENUM_SPECIFIER_DEFINITION, T_SYM_STR("enum"), NT_SYM(identifier_opt), T_SYM_STR("{"), NT_SYM(enumerator_list), T_SYM_STR("}")),
        ALT(ENUM_SPECIFIER_DEFINITION_TRAILING_COMMA, T_SYM_STR("enum"), NT_SYM(identifier_opt), T_SYM_STR("{"), NT_SYM(enumerator_list), T_SYM_STR(","), T_SYM_STR("}")),
        ALT(ENUM_SPECIFIER_DECLARATION, T_SYM_STR("enum"), NT_SYM(identifier)));

const struct production_rule enumerator_list = PR_RULE("enumerator-list",
        ALT(LIST_RULE_ONE, NT_SYM(enumerator)),
        ALT(LIST_RULE_MULTI, NT_SYM(enumerator_list), T_SYM_STR(","), NT_SYM(enumerator)));

const struct production_rule enumerator = PR_RULE("enumerator",
        ALT(ENUMERATOR_NO_ASSIGNMENT, NT_SYM(enumeration_constant)),
        ALT(ENUMERATOR_ASSIGNMENT, NT_SYM(enumeration_constant), T_SYM_STR("="), NT_SYM(constant_expression)));

const struct production_rule abstract_declarator = PR_RULE("abstract-declarator",
                                                           ALT(ABSTRACT_DECLARATOR_POINTER, NT_SYM(pointer)),
                                                           ALT(ABSTRACT_DECLARATOR_DIRECT, NT_SYM(direct_abstract_declarator_opt), NT_SYM(pointer_opt)));
const struct production_rule abstract_declarator_opt = OPT("abstract-declarator_opt", abstract_declarator);

// the C standard writes this as
// pointer:
//  * type-qualifier-list_opt
//  * type-qualifier-list_opt pointer
// and I'm not sure why
const struct production_rule pointer = PR_RULE("pointer",
                                               ALT(NO_TAG, T_SYM_STR("*"), NT_SYM(type_qualifier_list_opt), NT_SYM(pointer_opt)));
const struct production_rule pointer_opt = OPT("pointer_opt", pointer);

const struct production_rule type_qualifier = PR_RULE("type-qualifier",
                                                      ALT(TYPE_QUALIFIER_CONST, T_SYM_STR("const")),
                                                      ALT(TYPE_QUALIFIER_RESTRICT, T_SYM_STR("restrict")),
                                                      ALT(TYPE_QUALIFIER_VOLATILE, T_SYM_STR("volatile")));

const struct production_rule type_qualifier_list = PR_RULE("type-qualifier-list",
                                                          ALT(LIST_RULE_ONE, NT_SYM(type_qualifier)),
                                                          ALT(LIST_RULE_MULTI, NT_SYM(type_qualifier_list), NT_SYM(type_qualifier)));

const struct production_rule type_qualifier_list_opt = OPT("type-qualifier-list_opt", type_qualifier_list);

const struct production_rule direct_abstract_declarator = PR_RULE("direct-abstract-declarator",
                                                                  ALT(DIRECT_ABSTRACT_DECLARATOR_PARENS, NT_SYM(direct_abstract_declarator_opt), T_SYM_STR("("), NT_SYM(parameter_type_list_opt), T_SYM_STR(")")),
                                                                  ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY, NT_SYM(direct_abstract_declarator_opt), T_SYM_STR("["), NT_SYM(type_qualifier_list_opt), NT_SYM(assignment_expression_opt), T_SYM_STR("]")),
                                                                  ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_STATIC, NT_SYM(direct_abstract_declarator_opt), T_SYM_STR("["), T_SYM_STR("static"), NT_SYM(type_qualifier_list_opt), NT_SYM(assignment_expression), T_SYM_STR("]")),
                                                                  ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_QUALIFIER_STATIC, NT_SYM(direct_abstract_declarator_opt), T_SYM_STR("["), NT_SYM(type_qualifier_list), T_SYM_STR("static"), NT_SYM(assignment_expression), T_SYM_STR("]")),
                                                                  ALT(DIRECT_ABSTRACT_DECLARATOR_ARRAY_ASTERISK, NT_SYM(direct_abstract_declarator_opt), T_SYM_STR("["), T_SYM_STR("*"), T_SYM_STR("]")));
const struct production_rule direct_abstract_declarator_opt = OPT("direct_abstract_declarator_opt", direct_abstract_declarator);

const struct production_rule parameter_type_list = PR_RULE("parameter-type-list",
                                                           ALT(PARAMETER_TYPE_LIST_ELLIPSIS, NT_SYM(parameter_list), T_SYM_STR(","), T_SYM_STR("...")),
                                                           ALT(PARAMETER_TYPE_LIST_NO_ELLIPSIS, NT_SYM(parameter_list)));
const struct production_rule parameter_type_list_opt = OPT("parameter-type-list_opt", parameter_type_list);

const struct production_rule parameter_list = PR_RULE("parameter-list",
                                                        ALT(LIST_RULE_ONE, NT_SYM(parameter_declaration)),
                                                        ALT(LIST_RULE_MULTI, NT_SYM(parameter_list), T_SYM_STR(","), NT_SYM(parameter_declaration)));

const struct production_rule parameter_declaration = PR_RULE("parameter-declaration",
                                                            ALT(PARAMETER_DECLARATION_DECLARATOR, NT_SYM(declaration_specifiers), NT_SYM(declarator)),
                                                            ALT(PARAMETER_DECLARATION_ABSTRACT, NT_SYM(declaration_specifiers), NT_SYM(abstract_declarator_opt)));

const struct production_rule declaration_specifiers = PR_RULE("declaration-specifiers",
                                                                ALT(DECLARATION_SPECIFIERS_STORAGE_CLASS, NT_SYM(storage_class_specifier), NT_SYM(declaration_specifiers_opt)),
                                                                ALT(DECLARATION_SPECIFIERS_TYPE_SPECIFIER, NT_SYM(type_specifier), NT_SYM(declaration_specifiers_opt)),
                                                                ALT(DECLARATION_SPECIFIERS_TYPE_QUALIFIER, NT_SYM(type_qualifier), NT_SYM(declaration_specifiers_opt)),
                                                                ALT(DECLARATION_SPECIFIERS_FUNCTION_SPECIFIER, NT_SYM(function_specifier), NT_SYM(declaration_specifiers_opt)));
const struct production_rule declaration_specifiers_opt = OPT("declaration-specifiers_opt", declaration_specifiers);

const struct production_rule storage_class_specifier = PR_RULE("storage-class-specifier",
                                                                ALT(STORAGE_CLASS_SPECIFIER_TYPEDEF, T_SYM_STR("typedef")),
                                                                ALT(STORAGE_CLASS_SPECIFIER_EXTERN, T_SYM_STR("extern")),
                                                                ALT(STORAGE_CLASS_SPECIFIER_STATIC, T_SYM_STR("static")),
                                                                ALT(STORAGE_CLASS_SPECIFIER_AUTO, T_SYM_STR("auto")),
                                                                ALT(STORAGE_CLASS_SPECIFIER_REGISTER, T_SYM_STR("register")));

const struct production_rule function_specifier = PR_RULE("function-specifier", ALT(NO_TAG, T_SYM_STR("inline")));

const struct production_rule typedef_name = PR_RULE("typedef-name", ALT(NO_TAG, NT_SYM(identifier)));
