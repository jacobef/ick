#ifndef ICK_PARSER_H
#define ICK_PARSER_H

#include "data_structures/vector.h"
#include "preprocessor/pp_token.h"
#include <stddef.h>
#include <stdbool.h>
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

typedef struct symbol {
    union {
        const struct production_rule *rule;
        struct terminal terminal;
    } val;
    bool is_terminal;
} symbol;
DEFINE_HARR_TYPE_AND_FUNCTIONS(symbol)

typedef struct alternative {
    // represents e.g. B C D (in the context of A -> B C D | E F G)
    symbol_harr symbols;
    int tag;
} alternative;
DEFINE_HARR_TYPE_AND_FUNCTIONS(alternative)

struct production_rule {
    // represents e.g. A -> B C D | E F G
    const char *name;
    alternative_harr alternatives;
    bool is_list_rule;
};

typedef struct earley_rule *erule_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p)

struct earley_rule {
    // represents e.g. A -> B [dot] C D (in the context of A -> B C D | E F G)
    const struct production_rule *lhs;
    struct alternative rhs;
    size_t dot;  // if dot is n, then it's "behind" the symbol at index n (i.e. rhs.symbols.data[n])
    const erule_p_harr *origin_chart;
    erule_p_harr completed_from;
};

typedef erule_p_vec *erule_p_vec_p;
typedef erule_p_harr *erule_p_harr_p;
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p_harr_p)
DEFINE_VEC_TYPE_AND_FUNCTIONS(erule_p_vec_p)
erule_p_harr_p_harr make_charts(pp_token_harr tokens, const struct production_rule *start_rule);

pp_token_harr pp_tokens_rule_as_harr(struct earley_rule pp_tokens_rule);

void print_chart(const erule_p_harr *chart);
void print_tree(const struct earley_rule *root, size_t indent);

struct earley_rule *parse(pp_token_harr tokens, const struct production_rule *root_rule);
struct earley_rule *parse_full_file(pp_token_harr tokens);

extern const struct production_rule tr_preprocessing_file;
extern const struct production_rule tr_group_opt;
extern const struct production_rule tr_group;
extern const struct production_rule tr_group_part;
extern const struct production_rule tr_if_section;
extern const struct production_rule tr_if_group;
extern const struct production_rule tr_elif_groups;
extern const struct production_rule tr_elif_groups_opt;
extern const struct production_rule tr_elif_group;
extern const struct production_rule tr_else_group;
extern const struct production_rule tr_else_group_opt;
extern const struct production_rule tr_endif_line;
extern const struct production_rule tr_control_line;
extern const struct production_rule tr_non_hashtag;
extern const struct production_rule tr_tokens_not_starting_with_hashtag;
extern const struct production_rule tr_tokens_not_starting_with_hashtag_opt;
extern const struct production_rule tr_text_line;
extern const struct production_rule tr_not_directive_name;
extern const struct production_rule tr_tokens_not_starting_with_directive_name;
extern const struct production_rule tr_non_directive;
extern const struct production_rule tr_lparen;
extern const struct production_rule tr_replacement_list;
extern const struct production_rule tr_pp_tokens;
extern const struct production_rule tr_pp_tokens_opt;
extern const struct production_rule tr_identifier_list;
extern const struct production_rule tr_identifier_list_opt;
extern const struct production_rule tr_identifier;
extern const struct production_rule tr_identifier_opt;
extern const struct production_rule tr_string_literal;

extern const struct production_rule tr_constant;
extern const struct production_rule tr_integer_constant;
extern const struct production_rule tr_character_constant;
extern const struct production_rule tr_floating_constant;
extern const struct production_rule tr_enumeration_constant;
extern const struct production_rule tr_constant_expression;
extern const struct production_rule tr_conditional_expression;
extern const struct production_rule tr_logical_or_expression;
extern const struct production_rule tr_logical_and_expression;
extern const struct production_rule tr_inclusive_or_expression;
extern const struct production_rule tr_exclusive_or_expression;
extern const struct production_rule tr_and_expression;
extern const struct production_rule tr_equality_expression;
extern const struct production_rule tr_relational_expression;
extern const struct production_rule tr_shift_expression;
extern const struct production_rule tr_additive_expression;
extern const struct production_rule tr_multiplicative_expression;
extern const struct production_rule tr_cast_expression;
extern const struct production_rule tr_unary_expression;
extern const struct production_rule tr_unary_operator;
extern const struct production_rule tr_postfix_expression;
extern const struct production_rule tr_primary_expression;
extern const struct production_rule tr_assignment_expression;
extern const struct production_rule tr_assignment_expression_opt;
extern const struct production_rule tr_assignment_operator;
extern const struct production_rule tr_expression;

extern const struct production_rule tr_initializer_list;
extern const struct production_rule tr_argument_expression_list;
extern const struct production_rule tr_argument_expression_list_opt;
extern const struct production_rule tr_designation;
extern const struct production_rule tr_designation_opt;
extern const struct production_rule tr_designator_list;
extern const struct production_rule tr_designator;
extern const struct production_rule tr_initializer;
extern const struct production_rule tr_abstract_declarator;
extern const struct production_rule tr_abstract_declarator_opt;
extern const struct production_rule tr_direct_abstract_declarator;
extern const struct production_rule tr_direct_abstract_declarator_opt;
extern const struct production_rule tr_parameter_type_list;
extern const struct production_rule tr_parameter_type_list_opt;
extern const struct production_rule tr_parameter_list;
extern const struct production_rule tr_parameter_declaration;
extern const struct production_rule tr_specifier_qualifier_list;
extern const struct production_rule tr_specifier_qualifier_list_opt;
extern const struct production_rule tr_declaration_specifiers;
extern const struct production_rule tr_declaration_specifiers_opt;
extern const struct production_rule tr_storage_class_specifier;
extern const struct production_rule tr_function_specifier;
extern const struct production_rule tr_type_name;
extern const struct production_rule tr_type_specifier;
extern const struct production_rule tr_struct_or_union_specifier;
extern const struct production_rule tr_struct_or_union;
extern const struct production_rule tr_struct_declaration_list;
extern const struct production_rule tr_struct_declaration;
extern const struct production_rule tr_struct_declarator_list;
extern const struct production_rule tr_struct_declarator;
extern const struct production_rule tr_declarator;
extern const struct production_rule tr_declarator_opt;
extern const struct production_rule tr_direct_declarator;
extern const struct production_rule tr_enum_specifier;
extern const struct production_rule tr_enumerator_list;
extern const struct production_rule tr_enumerator;
extern const struct production_rule tr_type_qualifier;
extern const struct production_rule tr_type_qualifier_list;
extern const struct production_rule tr_type_qualifier_list_opt;
extern const struct production_rule tr_typedef_name;
extern const struct production_rule tr_pointer;
extern const struct production_rule tr_pointer_opt;

extern const struct production_rule tr_preprocessing_token;

enum opt_tag { OPT_ONE, OPT_NONE };

enum list_rule_tag { LIST_RULE_ONE, LIST_RULE_MULTI };

// group-part: if-section control-line text-line # non-directive
enum group_part_tag { GROUP_PART_IF, GROUP_PART_CONTROL, GROUP_PART_TEXT, GROUP_PART_NON_DIRECTIVE };

// if-group: # if constant-expression new-line group_opt
//           # ifdef identifier new-line group_opt
//           # ifndef identifier new-line group_opt
enum if_group_tag { IF_GROUP_IF, IF_GROUP_IFDEF, IF_GROUP_IFNDEF };


enum cond_expr_tag { COND_EXPR_LOGICAL_OR, COND_EXPR_NORMAL };

enum constant_tag {
    CONSTANT_INTEGER,
    CONSTANT_CHARACTER,
    CONSTANT_FLOAT,
    CONSTANT_ENUM
};

enum logical_or_expr_tag {
    LOGICAL_OR_EXPR_LOGICAL_AND,
    LOGICAL_OR_EXPR_NORMAL
};

enum logical_and_expr_tag {
    LOGICAL_AND_EXPR_INCLUSIVE_OR,
    LOGICAL_AND_EXPR_NORMAL
};

enum inclusive_or_expr_tag {
    INCLUSIVE_OR_EXPR_EXCLUSIVE_OR,
    INCLUSIVE_OR_EXPR_NORMAL
};

enum exclusive_or_expr_tag {
    EXCLUSIVE_OR_EXPR_AND,
    EXCLUSIVE_OR_EXPR_NORMAL
};

enum and_expr_tag {
    AND_EXPR_EQUALITY,
    AND_EXPR_NORMAL
};

enum equality_expr_tag {
    EQUALITY_EXPR_RELATIONAL,
    EQUALITY_EXPR_EQUAL,
    EQUALITY_EXPR_NOT_EQUAL
};

enum relational_expr_tag {
    RELATIONAL_EXPR_SHIFT,
    RELATIONAL_EXPR_LESS,
    RELATIONAL_EXPR_GREATER,
    RELATIONAL_EXPR_LEQ,
    RELATIONAL_EXPR_GEQ
};

enum shift_expr_tag {
    SHIFT_EXPR_ADDITIVE,
    SHIFT_EXPR_LEFT,
    SHIFT_EXPR_RIGHT
};

enum add_expr_tag {
    ADDITIVE_EXPR_MULT,
    ADDITIVE_EXPR_PLUS,
    ADDITIVE_EXPR_MINUS
};

enum mult_expr_tag {
    MULTIPLICATIVE_EXPR_CAST,
    MULTIPLICATIVE_EXPR_MULT,
    MULTIPLICATIVE_EXPR_DIV,
    MULTIPLICATIVE_EXPR_MOD
};

enum cast_expr_tag {
    CAST_EXPR_UNARY,
    CAST_EXPR_NORMAL
};

enum unary_expr_tag {
    UNARY_EXPR_POSTFIX,
    UNARY_EXPR_INC,
    UNARY_EXPR_DEC,
    UNARY_EXPR_UNARY_OP,
    UNARY_EXPR_SIZEOF_UNARY,
    UNARY_EXPR_SIZEOF_TYPE
};

enum unary_operator_tag {
    UNARY_OPERATOR_PLUS,
    UNARY_OPERATOR_MINUS,
    UNARY_OPERATOR_BITWISE_NOT,
    UNARY_OPERATOR_LOGICAL_NOT,
    UNARY_OPERATOR_DEREFERENCE,
    UNARY_OPERATOR_ADDRESS_OF
};

enum assignment_operator_tag {
    ASSIGNMENT_OPERATOR_ASSIGN,
    ASSIGNMENT_OPERATOR_MULTIPLY_ASSIGN,
    ASSIGNMENT_OPERATOR_DIVIDE_ASSIGN,
    ASSIGNMENT_OPERATOR_MODULO_ASSIGN,
    ASSIGNMENT_OPERATOR_ADD_ASSIGN,
    ASSIGNMENT_OPERATOR_SUBTRACT_ASSIGN,
    ASSIGNMENT_OPERATOR_SHIFT_LEFT_ASSIGN,
    ASSIGNMENT_OPERATOR_SHIFT_RIGHT_ASSIGN,
    ASSIGNMENT_OPERATOR_BITWISE_AND_ASSIGN,
    ASSIGNMENT_OPERATOR_BITWISE_XOR_ASSIGN,
    ASSIGNMENT_OPERATOR_BITWISE_OR_ASSIGN
};

enum postfix_expr_tag {
    POSTFIX_EXPR_PRIMARY,
    POSTFIX_EXPR_ARRAY_ACCESS,
    POSTFIX_EXPR_FUNC,
    POSTFIX_EXPR_DOT,
    POSTFIX_EXPR_ARROW,
    POSTFIX_EXPR_INC,
    POSTFIX_EXPR_DEC,
    POSTFIX_EXPR_COMPOUND_LITERAL
};

enum primary_expr_tag {
    PRIMARY_EXPR_IDENTIFIER,
    PRIMARY_EXPR_CONSTANT,
    PRIMARY_EXPR_STRING,
    PRIMARY_EXPR_PARENS
};

enum assignment_expr_tag {
    ASSIGNMENT_EXPR_CONDITIONAL,
    ASSIGNMENT_EXPR_NORMAL
};

enum designator_tag {
    DESIGNATOR_ARRAY,
    DESIGNATOR_DOT
};

enum initializer_tag {
    INITIALIZER_ASSIGNMENT,
    INITIALIZER_BRACES,
    INITIALIZER_BRACES_TRAILING_COMMA
};

enum direct_declarator_tag {
    DIRECT_DECLARATOR_IDENTIFIER,
    DIRECT_DECLARATOR_PARENS,
    DIRECT_DECLARATOR_ARRAY,
    DIRECT_DECLARATOR_ARRAY_STATIC,
    DIRECT_DECLARATOR_ARRAY_STATIC_2,
    DIRECT_DECLARATOR_ARRAY_ASTERISK,
    DIRECT_DECLARATOR_FUNCTION,
    DIRECT_DECLARATOR_FUNCTION_OLD
};

enum abstract_declarator_tag {
    ABSTRACT_DECLARATOR_POINTER,
    ABSTRACT_DECLARATOR_DIRECT
};

enum direct_abstract_declarator_tag {
    DIRECT_ABSTRACT_DECLARATOR_PARENS,
    DIRECT_ABSTRACT_DECLARATOR_ARRAY,
    DIRECT_ABSTRACT_DECLARATOR_ARRAY_STATIC,
    DIRECT_ABSTRACT_DECLARATOR_ARRAY_QUALIFIER_STATIC,
    DIRECT_ABSTRACT_DECLARATOR_ARRAY_ASTERISK
};

enum specifier_qualifier_list_tag {
    SPECIFIER_QUALIFIER_LIST_SPECIFIER,
    SPECIFIER_QUALIFIER_LIST_QUALIFIER
};

enum type_specifier_tag {
    TYPE_SPECIFIER_VOID,
    TYPE_SPECIFIER_CHAR,
    TYPE_SPECIFIER_SHORT,
    TYPE_SPECIFIER_INT,
    TYPE_SPECIFIER_LONG,
    TYPE_SPECIFIER_FLOAT,
    TYPE_SPECIFIER_DOUBLE,
    TYPE_SPECIFIER_SIGNED,
    TYPE_SPECIFIER_UNSIGNED,
    TYPE_SPECIFIER_BOOL,
    TYPE_SPECIFIER_COMPLEX,
    TYPE_SPECIFIER_STRUCT_OR_UNION,
    TYPE_SPECIFIER_ENUM,
    TYPE_SPECIFIER_TYPEDEF_NAME
};

enum struct_or_union_specifier_tag {
    STRUCT_OR_UNION_SPECIFIER_DEFINITION,
    STRUCT_OR_UNION_SPECIFIER_DECLARATION
};

enum struct_or_union_tag {
    STRUCT_OR_UNION_STRUCT,
    STRUCT_OR_UNION_UNION
};

enum struct_declarator_tag {
    STRUCT_DECLARATOR_NORMAL,
    STRUCT_DECLARATOR_BITFIELD
};

enum enum_specifier_tag {
    ENUM_SPECIFIER_DEFINITION,
    ENUM_SPECIFIER_DEFINITION_TRAILING_COMMA,
    ENUM_SPECIFIER_DECLARATION
};

enum enumerator_tag {
    ENUMERATOR_NO_ASSIGNMENT,
    ENUMERATOR_ASSIGNMENT
};

enum type_qualifier_tag {
    TYPE_QUALIFIER_CONST,
    TYPE_QUALIFIER_RESTRICT,
    TYPE_QUALIFIER_VOLATILE
};

enum parameter_type_list_tag {
    PARAMETER_TYPE_LIST_ELLIPSIS,
    PARAMETER_TYPE_LIST_NO_ELLIPSIS,
};

enum parameter_declaration_tag {
    PARAMETER_DECLARATION_DECLARATOR,
    PARAMETER_DECLARATION_ABSTRACT,
};

enum declaration_specifiers_tag {
    DECLARATION_SPECIFIERS_STORAGE_CLASS,
    DECLARATION_SPECIFIERS_TYPE_SPECIFIER,
    DECLARATION_SPECIFIERS_TYPE_QUALIFIER,
    DECLARATION_SPECIFIERS_FUNCTION_SPECIFIER
};

enum storage_class_specifier_tag {
    STORAGE_CLASS_SPECIFIER_TYPEDEF,
    STORAGE_CLASS_SPECIFIER_EXTERN,
    STORAGE_CLASS_SPECIFIER_STATIC,
    STORAGE_CLASS_SPECIFIER_AUTO,
    STORAGE_CLASS_SPECIFIER_REGISTER
};

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
