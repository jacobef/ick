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
#define ABC wjefiowjfoiwej

static bool match_preprocessing_token(__attribute__((unused)) struct preprocessing_token token) {
    return !token_is_str(token, (unsigned char*)"\n");
}

static bool match_lparen(struct preprocessing_token token) {
    return token_is_str(token, (unsigned char*)"(") && !token.after_whitespace;
}
