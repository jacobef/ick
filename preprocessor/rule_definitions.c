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

// preprocessing-file: group_opt
const struct production_rule preprocessing_file = PR_RULE("preprocessing-file", ALT(NO_TAG, NT_SYM(group_opt)));

const struct production_rule group_opt = OPT("group_opt", group);

// group: group-part group group-part
const struct production_rule group = PR_RULE("group",
                                             ALT(LIST_RULE_ONE, NT_SYM(group_part)),
                                             ALT(LIST_RULE_MULTI, NT_SYM(group), NT_SYM(group_part)));

// group-part: if-section control-line text-line # non-directive
const struct production_rule group_part = PR_RULE("group-part",
                                                  ALT(GROUP_PART_IF, NT_SYM(if_section)),
                                                  ALT(GROUP_PART_CONTROL, NT_SYM(control_line)),
                                                  ALT(GROUP_PART_TEXT, NT_SYM(text_line)),
                                                  ALT(GROUP_PART_NON_DIRECTIVE, T_SYM_STR("#"), NT_SYM(non_directive)));

// if-section: if-group elif-groups_opt else-group_opt endif-line
const struct production_rule if_section = PR_RULE("if-section",
                                                  ALT(NO_TAG, NT_SYM(if_group), /*NT_SYM(elif_groups),*/ NT_SYM(else_group_opt), NT_SYM(endif_line)));

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
const struct production_rule if_group = PR_RULE("if-group",
//                                                 ALT(T_SYM_STR("#"), T_SYM_STR("if"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                ALT(IF_GROUP_IFDEF, T_SYM_STR("#"), T_SYM_STR("ifdef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)),
                                                ALT(IF_GROUP_IFNDEF, T_SYM_STR("#"), T_SYM_STR("ifndef"), NT_SYM(identifier), T_SYM_STR("\n"), NT_SYM(group_opt)));

//// elif-groups: elif-group
////              elif-groups elif-group
//const struct production_rule elif_groups = PR_RULE("elif_groups",
//                                                    ALT(NT_SYM(elif_group)),
//                                                    ALT(NT_SYM(elif_groups), NT_SYM(elif_group)));

//// elif-group: # elif constant-expression new-line group_opt
//const struct production_rule elif_group = PR_RULE("elif_group",
//                                                   ALT(T_SYM_STR("#"), T_SYM_STR("elif"), NT_SYM(constant_expression), T_SYM_STR("\n"), NT_SYM(group_opt)));

// else-group: # else new-line group_opt
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
