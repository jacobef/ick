#ifndef ICK_PARSER_H
#define ICK_PARSER_H

#include "data_structures/vector.h"
#include "preprocessor/detector.h"
#include "preprocessor/pp_token.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum terminal_symbol_type {
    TERMINAL_FN, TERMINAL_STR
};

struct terminal {
    union {
        bool (*fn)(struct preprocessing_token);
        unsigned char *str;
    } matcher;
    enum terminal_symbol_type type;
    struct preprocessing_token token;
    bool is_filled;
};

struct symbol {
    union {
        struct production_rule *rule;
        struct terminal terminal;
    } val;
    bool is_terminal;
};

struct alternative {
    struct symbol *symbols;
    size_t n;
    int tag;
};

struct production_rule {
    const char *name;
    struct alternative *alternatives;
    size_t n;
};

typedef struct earley_rule *erule_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p)

struct earley_rule {
    struct production_rule *lhs;
    struct alternative rhs;
    struct symbol *dot;
    erule_p_vec *origin_chart;
    erule_p_vec completed_from;
};

typedef erule_p_vec *erule_p_vec_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p_vec_p)
erule_p_vec_p_vec make_charts(pp_token_vec tokens);

void print_chart(erule_p_vec *chart);
void print_tree(struct earley_rule *root, size_t indent);
void test_parser(pp_token_vec tokens);

#define ALT(_tag, ...)                                                   \
    ((struct alternative) {                                              \
        .symbols=(struct symbol[]) {__VA_ARGS__},                        \
        .n=sizeof((struct symbol[]){__VA_ARGS__})/sizeof(struct symbol), \
        .tag=_tag                                                        \
    })

#define PR_RULE(_name, ...)                                                       \
    ((struct production_rule) {                                                   \
        .name=_name,                                                              \
        .alternatives=(struct alternative[]) {__VA_ARGS__},                       \
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

#define EMPTY_ALT(_tag) \
    ((struct alternative) { \
        .symbols=(struct symbol[]) { 0 }, \
        .n=0, \
        .tag=_tag       \
    })

enum opt_tag { OPT_ONE, OPT_NONE };
#define OPT(_rule) PR_RULE(#_rule "_opt", ALT(OPT_ONE, NT_SYM(_rule)), EMPTY_ALT(OPT_NONE))

static bool match_preprocessing_token(__attribute__((unused)) struct preprocessing_token token) {
    return !token_is_str(token, (unsigned char*)"\n");
}

static bool match_lparen(struct preprocessing_token token) {
    return token_is_str(token, (unsigned char*)"(") && !token.after_whitespace;
}

static bool match_identifier(struct preprocessing_token token) {
    return token.type == IDENTIFIER;
}

static bool match_non_hashtag(struct preprocessing_token token) {
    return match_preprocessing_token(token) && !token_is_str(token, (unsigned char*)"#");
}

static bool match_non_directive_name(struct preprocessing_token token) {
    return match_preprocessing_token(token) &&
           !token_is_str(token, (unsigned char*)"define") &&
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

// Forward Declarations
static struct production_rule preprocessing_file;
static struct production_rule group_opt;
static struct production_rule group;
static struct production_rule group_part;
static struct production_rule if_section;
static struct production_rule if_group;
static struct production_rule elif_groups;
static struct production_rule elif_group;
static struct production_rule else_group;
static struct production_rule else_group_opt;
static struct production_rule endif_line;
static struct production_rule control_line;
static struct production_rule non_hashtag;
static struct production_rule tokens_not_starting_with_hashtag;
static struct production_rule tokens_not_starting_with_hashtag_opt;
static struct production_rule text_line;
static struct production_rule not_a_directive_name;
static struct production_rule tokens_not_starting_with_directive_name;
static struct production_rule non_directive;
static struct production_rule lparen;
static struct production_rule replacement_list;
static struct production_rule pp_tokens;
static struct production_rule pp_tokens_opt;
static struct production_rule identifier_list;
static struct production_rule identifier_list_opt;
static struct production_rule identifier;

static struct production_rule constant_expression;
static struct production_rule conditional_expression;
static struct production_rule logical_or_expression;
static struct production_rule logical_and_expression;
static struct production_rule inclusive_or_expression;
static struct production_rule exclusive_or_expression;
static struct production_rule and_expression;
static struct production_rule equality_expression;
static struct production_rule relational_expression;
static struct production_rule shift_expression;
static struct production_rule additive_expression;
static struct production_rule multiplicative_expression;
static struct production_rule cast_expression;
static struct production_rule unary_expression;

static struct production_rule rule_preprocessing_token;

#define NO_TAG -1

// preprocessing-file: group_opt
static struct production_rule preprocessing_file = PR_RULE("preprocessing-file", ALT(NO_TAG, NT_SYM(group_opt)));

static struct production_rule group_opt = OPT(group);

enum list_rule_tag { LIST_RULE_ONE, LIST_RULE_MULTI };
// group: group-part group group-part
static struct production_rule group = PR_RULE("group",
                                              ALT(LIST_RULE_ONE, NT_SYM(group_part)),
                                              ALT(LIST_RULE_MULTI, NT_SYM(group), NT_SYM(group_part)));

// group-part: if-section control-line text-line # non-directive
enum group_part_tag { GROUP_PART_IF, GROUP_PART_CONTROL, GROUP_PART_TEXT, GROUP_PART_NON_DIRECTIVE };
static struct production_rule group_part = PR_RULE("group-part",
                                                   ALT(GROUP_PART_IF, NT_SYM(if_section)),
                                                   ALT(GROUP_PART_CONTROL, NT_SYM(control_line)),
                                                   ALT(GROUP_PART_TEXT, NT_SYM(text_line)),
                                                   ALT(GROUP_PART_NON_DIRECTIVE, T_SYM_STR("#"), NT_SYM(non_directive)));

// if-section: if-group elif-groups_opt else-group_opt endif-line
static struct production_rule if_section = PR_RULE("if-section",
                                                   ALT(NO_TAG, NT_SYM(if_group), /*NT_SYM(elif_groups),*/ NT_SYM(else_group_opt), NT_SYM(endif_line)));

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
enum if_group_tag { IF_GROUP_IF, IF_GROUP_IFDEF, IF_GROUP_IFNDEF };
static struct production_rule if_group = PR_RULE("if-group",
//                                                 ALT(T_SYM_STR("#"), T_SYM_STR("if"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                 ALT(IF_GROUP_IFDEF, T_SYM_STR("#"), T_SYM_STR("ifdef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                 ALT(IF_GROUP_IFNDEF, T_SYM_STR("#"), T_SYM_STR("ifndef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)));

//// elif-groups: elif-group
////              elif-groups elif-group
//static struct production_rule elif_groups = PR_RULE("elif_groups",
//                                                    ALT(NT_SYM(elif_group)),
//                                                    ALT(NT_SYM(elif_groups), NT_SYM(elif_group)));

//// elif-group: # elif constant-expression new-line group_opt
//static struct production_rule elif_group = PR_RULE("elif_group",
//                                                   ALT(T_SYM_STR("#"), T_SYM_STR("elif"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)));

// else-group: # else new-line group_opt
static struct production_rule else_group = PR_RULE("else-group",
                                                   ALT(NO_TAG, T_SYM_STR("#"), T_SYM_STR("else"), T_SYM_STR("\n"), NT_SYM(group_opt)));
static struct production_rule else_group_opt = OPT(else_group);

// endif-line: # endif new-line
static struct production_rule endif_line = PR_RULE("endif-line",
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
enum control_line_tag {
    CONTROL_LINE_INCLUDE,
    CONTROL_LINE_DEFINE_OBJECT_LIKE,
    CONTROL_LINE_DEFINE_FUNCTION_LIKE_NO_VARARGS,
    CONTROL_LINE_DEFINE_FUNCTION_LIKE_ONLY_VARARGS,
    CONTROL_LINE_DEFINE_FUNCTION_LIKE_MIXED_ARGS,
    CONTROL_LINE_UNDEF,
    CONTROL_LINE_LINE,
    CONTROL_LINE_ERROR,
    CONTROL_LINE_PRAGMA,
    CONTROL_LINE_EMPTY
};
static struct production_rule control_line = PR_RULE("control-line",
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
static struct production_rule non_hashtag = PR_RULE("non-hashtag", ALT(NO_TAG, T_SYM_FN(match_non_hashtag)));

static struct production_rule tokens_not_starting_with_hashtag = PR_RULE("tokens-not-starting-with-hashtag",
        ALT(LIST_RULE_ONE, NT_SYM(non_hashtag)),
        ALT(LIST_RULE_MULTI, NT_SYM(tokens_not_starting_with_hashtag), NT_SYM(rule_preprocessing_token)));
static struct production_rule tokens_not_starting_with_hashtag_opt = OPT(tokens_not_starting_with_hashtag);
static struct production_rule text_line = PR_RULE("text-line", ALT(NO_TAG, NT_SYM(tokens_not_starting_with_hashtag_opt), T_SYM_STR("\n")));

// non-directive: pp-tokens new-line
static struct production_rule not_directive_name = PR_RULE("not-directive-name", ALT(NO_TAG, T_SYM_FN(match_non_directive_name)));
static struct production_rule tokens_not_starting_with_directive_name = PR_RULE("tokens-not-starting-with-directive-name",
        ALT(LIST_RULE_ONE, NT_SYM(not_directive_name)),
        ALT(LIST_RULE_MULTI, NT_SYM(tokens_not_starting_with_directive_name), NT_SYM(rule_preprocessing_token)));
static struct production_rule non_directive = PR_RULE("non-directive",
                                                      ALT(NO_TAG, NT_SYM(tokens_not_starting_with_directive_name), T_SYM_STR("\n")));

// lparen: a ( character not immediately preceded by white-space
static struct production_rule lparen = PR_RULE("lparen", ALT(NO_TAG, T_SYM_FN(match_lparen)));

// replacement-list: pp-tokens_opt
static struct production_rule replacement_list = PR_RULE("replacement-list", ALT(NO_TAG, NT_SYM(pp_tokens_opt)));

// pp-tokens: preprocessing-token pp-tokens preprocessing-token
static struct production_rule pp_tokens = PR_RULE("pp-tokens",
                                                  ALT(LIST_RULE_ONE, NT_SYM(rule_preprocessing_token)),
                                                  ALT(LIST_RULE_MULTI, NT_SYM(pp_tokens), NT_SYM(rule_preprocessing_token)));

static struct production_rule pp_tokens_opt = OPT(pp_tokens);

static struct production_rule rule_preprocessing_token = PR_RULE("preprocessing-token", ALT(NO_TAG, T_SYM_FN(match_preprocessing_token)));

// identifier-list:
//      identifier
//      identifier-list , identifier
static struct production_rule identifier_list = PR_RULE("identifier-list",
                                                        ALT(LIST_RULE_ONE, NT_SYM(identifier)),
                                                        ALT(LIST_RULE_MULTI, NT_SYM(identifier_list), T_SYM_STR(","), NT_SYM(identifier)));
static struct production_rule identifier_list_opt = OPT(identifier_list);

static struct production_rule identifier = PR_RULE("identifier", ALT(NO_TAG, T_SYM_FN(match_identifier)));




#endif //ICK_PARSER_H
