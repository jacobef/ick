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
        const struct production_rule *rule;
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
    const struct alternative *alternatives;
    const size_t n;
};

typedef struct earley_rule *erule_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p)

struct earley_rule {
    const struct production_rule *lhs;
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

// Forward Declarations
extern const struct production_rule preprocessing_file;
extern const struct production_rule group_opt;
extern const struct production_rule group;
extern const struct production_rule group_part;
extern const struct production_rule if_section;
extern const struct production_rule if_group;
extern const struct production_rule elif_groups;
extern const struct production_rule elif_group;
extern const struct production_rule else_group;
extern const struct production_rule else_group_opt;
extern const struct production_rule endif_line;
extern const struct production_rule control_line;
extern const struct production_rule non_hashtag;
extern const struct production_rule tokens_not_starting_with_hashtag;
extern const struct production_rule tokens_not_starting_with_hashtag_opt;
extern const struct production_rule text_line;
extern const struct production_rule not_directive_name;
extern const struct production_rule tokens_not_starting_with_directive_name;
extern const struct production_rule non_directive;
extern const struct production_rule lparen;
extern const struct production_rule replacement_list;
extern const struct production_rule pp_tokens;
extern const struct production_rule pp_tokens_opt;
extern const struct production_rule identifier_list;
extern const struct production_rule identifier_list_opt;
extern const struct production_rule identifier;

extern const struct production_rule constant_expression;
extern const struct production_rule conditional_expression;
extern const struct production_rule logical_or_expression;
extern const struct production_rule logical_and_expression;
extern const struct production_rule inclusive_or_expression;
extern const struct production_rule exclusive_or_expression;
extern const struct production_rule and_expression;
extern const struct production_rule equality_expression;
extern const struct production_rule relational_expression;
extern const struct production_rule shift_expression;
extern const struct production_rule additive_expression;
extern const struct production_rule multiplicative_expression;
extern const struct production_rule cast_expression;
extern const struct production_rule unary_expression;

extern const struct production_rule rule_preprocessing_token;

#define NO_TAG -1

enum opt_tag { OPT_ONE, OPT_NONE };

enum list_rule_tag { LIST_RULE_ONE, LIST_RULE_MULTI };

// group-part: if-section control-line text-line # non-directive
enum group_part_tag { GROUP_PART_IF, GROUP_PART_CONTROL, GROUP_PART_TEXT, GROUP_PART_NON_DIRECTIVE };

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
enum if_group_tag { IF_GROUP_IF, IF_GROUP_IFDEF, IF_GROUP_IFNDEF };

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


#endif //ICK_PARSER_H
