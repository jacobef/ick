#ifndef ICK_PARSER_H
#define ICK_PARSER_H

#include "data_structures/vector.h"
#include "preprocessor/detector.h"
#include "preprocessor/pp_token.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

struct symbol {
    union {
        struct production_rule *rule;
        bool (*terminal)(struct preprocessing_token);
    } val;
    bool is_terminal;
};

struct alternative {
    struct symbol *symbols;
    size_t n;
};

struct production_rule {
    const char *name;
    struct alternative *alternatives;
    size_t n;
};

#define ALT_SYM(...) \
    (struct alternative) { \
        .symbols=(struct symbol[]) {__VA_ARGS__}, \
        .n=sizeof((struct symbol[]){__VA_ARGS__})/sizeof(struct symbol) \
    }

#define PR_RULE(_name, ...) \
    (struct production_rule) { \
        .name=_name, \
        .alternatives=(struct alternative[]) {__VA_ARGS__}, \
        .n=sizeof((struct alternative[]){__VA_ARGS__})/sizeof(struct alternative) \
    }

#define NT_SYM(_rule) \
    (struct symbol) { \
        .val.rule=&(_rule), \
        .is_terminal=false \
    }

#define T_SYM(_fn) \
    (struct symbol) { \
        .val.terminal=_fn, \
        .is_terminal=true \
    }

#define EMPTY_ALT \
    ((struct alternative) { \
        .symbols=(struct symbol[]) { 0 }, \
        .n=0 \
    })

#define OPT(_rule) PR_RULE(#_rule "_opt", ALT_SYM(NT_SYM(_rule)), EMPTY_ALT)


static bool match_any_token(__attribute__((unused)) struct preprocessing_token token) {
    return true;
}

static bool match_lparen(struct preprocessing_token token) {
    return token_is_str(token, (unsigned char*)"(") && !token.after_whitespace;
}

static bool match_new_line(struct preprocessing_token token) {
    return token_is_str(token, (unsigned char*)"\n");
}


static struct production_rule preprocessing_token = PR_RULE("preprocessing_token", ALT_SYM(T_SYM(match_any_token)));

static struct production_rule pp_tokens = PR_RULE("pp_tokens",
        ALT_SYM(NT_SYM(preprocessing_token)),
        ALT_SYM(NT_SYM(pp_tokens), NT_SYM(preprocessing_token)));

 static struct production_rule pp_tokens_opt = OPT(pp_tokens);

 static struct production_rule lparen = PR_RULE("lparen", ALT_SYM(T_SYM(match_lparen)));

 static struct production_rule new_line = PR_RULE("new_line", ALT_SYM(T_SYM(match_new_line)));
//
// static struct production_rule text_line = PR_1_ALT("text_line", ((struct alternative) {
//     .symbols=(struct symbol[]) {
//         NT_SYM(pp_tokens_opt),
//         NT_SYM(new_line)
//     },
//     .n=2
// }));
//
//
// static struct production_rule non_directive = PR_1_ALT("non_directive", ((struct alternative) {
//     .symbols=(struct symbol[]) {
//         NT_SYM(pp_tokens),
//         NT_SYM(new_line)
//     },
//     .n=2
// }));

#endif //ICK_PARSER_H
