#include <math.h>
#include <limits.h>

#include "conditional_inclusion.h"
#include "preprocessor/diagnostics.h"
#include "preprocessor/char_parser.h"
#include "debug/color_print.h"
#include "mappings/typedefs.h"

static inline struct maybe_signed_intmax msi_s(target_intmax_t num) {
    return (struct maybe_signed_intmax) { .is_signed = true, .val.signd = num };
}

static inline struct maybe_signed_intmax msi_u(target_uintmax_t num) {
    return (struct maybe_signed_intmax) { .is_signed = false, .val.unsignd = num };
}

#define MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(msi1, msi2, op) \
if (!(msi1).is_signed && !(msi2).is_signed) {                       \
    return msi_u((msi1).val.unsignd op (msi2).val.unsignd);         \
} else if ((msi1).is_signed && !(msi2).is_signed) {                 \
    return msi_u((msi1).val.signd op (msi2).val.unsignd);           \
} else if (!(msi1).is_signed && (msi2).is_signed) {                 \
    return msi_u((msi1).val.unsignd op (msi2).val.signd);           \
} else {                                                            \
    return msi_s((msi1).val.signd op (msi2).val.signd);             \
}

#define MSI_BINARY_OP_RETURN_SIGNED_RESULT(msi1, msi2, op)  \
if (!(msi1).is_signed && !(msi2).is_signed) {               \
    return msi_s((msi1).val.unsignd op (msi2).val.unsignd); \
} else if ((msi1).is_signed && !(msi2).is_signed) {         \
    return msi_s((msi1).val.signd op (msi2).val.unsignd);   \
} else if (!(msi1).is_signed && (msi2).is_signed) {         \
    return msi_s((msi1).val.unsignd op (msi2).val.signd);   \
} else {                                                    \
    return msi_s((msi1).val.signd op (msi2).val.signd);     \
}


#define EVAL_1_ALT_BINARY_OP_USUAL_CONVERSIONS(name, fallthrough_func, op)                                                            \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                   \
    if (rule.rhs.tag == 0) {                                                                                        \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                       \
    } else if (rule.rhs.tag == 1) {                                                                                 \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op); \
    } else {                                                                                                        \
        preprocessor_fatal_error(0, 0, 0, "simple binary op tag is not 0 or 1");                                    \
    }                                                                                                               \
}

#define EVAL_1_ALT_BINARY_OP_SIGNED_RESULT(name, fallthrough_func, op)                                              \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                   \
    if (rule.rhs.tag == 0) {                                                                                        \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                       \
    } else if (rule.rhs.tag == 1) {                                                                                 \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op); \
    } else {                                                                                                        \
        preprocessor_fatal_error(0, 0, 0, "simple binary op tag is not 0 or 1");                                    \
    }                                                                                                               \
}

#define EVAL_2_ALT_BINARY_OP_USUAL_CONVERSIONS(name, fallthrough_func, op1, op2)                                                       \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                    \
    if (rule.rhs.tag == 0) {                                                                                         \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                        \
    } else if (rule.rhs.tag == 1) {                                                                                  \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op1); \
    } else if (rule.rhs.tag == 2) {                                                                                  \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op2); \
    } else {                                                                                                         \
        preprocessor_fatal_error(0, 0, 0, "2 alt binary op tag is not 0, 1, or 2");                                  \
    }                                                                                                                \
}

#define EVAL_2_ALT_BINARY_OP_SIGNED_RESULT(name, fallthrough_func, op1, op2)                                         \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                    \
    if (rule.rhs.tag == 0) {                                                                                         \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                        \
    } else if (rule.rhs.tag == 1) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op1); \
    } else if (rule.rhs.tag == 2) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op2); \
    } else {                                                                                                         \
        preprocessor_fatal_error(0, 0, 0, "2 alt binary op tag is not 0, 1, or 2");                                  \
    }                                                                                                                \
}

#define EVAL_3_ALT_BINARY_OP_USUAL_CONVERSIONS(name, fallthrough_func, op1, op2, op3)                                                  \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                    \
    if (rule.rhs.tag == 0) {                                                                                         \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                        \
    } else if (rule.rhs.tag == 1) {                                                                                  \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op1); \
    } else if (rule.rhs.tag == 2) {                                                                                  \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op2); \
    } else if (rule.rhs.tag == 3) {                                                                                  \
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op3); \
    } else {                                                                                                         \
        preprocessor_fatal_error(0, 0, 0, "3 alt binary op tag is not 0, 1, 2, or 3");                               \
    }                                                                                                                \
}

#define EVAL_4_ALT_BINARY_OP_SIGNED_RESULT(name, fallthrough_func, op1, op2, op3, op4)                                             \
static struct maybe_signed_intmax name(struct earley_rule rule) {                                                    \
    if (rule.rhs.tag == 0) {                                                                                         \
        return fallthrough_func(*rule.completed_from.arr[0]);                                                        \
    } else if (rule.rhs.tag == 1) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op1); \
    } else if (rule.rhs.tag == 2) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op2); \
    } else if (rule.rhs.tag == 3) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op3); \
    } else if (rule.rhs.tag == 4) {                                                                                  \
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(name(*rule.completed_from.arr[0]), fallthrough_func(*rule.completed_from.arr[1]), op4); \
    } else {                                                                                                         \
        preprocessor_fatal_error(0, 0, 0, "4 alt binary op tag is not 0, 1, 2, 3, or 4");                            \
    }                                                                                                                \
}


static unsigned char get_ascii_value(unsigned char c) {
    // TODO hardcode all possible values so this doesn't depend on the compiler
    return c;
}

static unsigned char get_hex_digit_value(unsigned char digit) {
    if ('0' <= digit && digit <= '9') return digit - '0';
    else if (digit == 'a' || digit == 'A') return 10;
    else if (digit == 'b' || digit == 'B') return 11;
    else if (digit == 'c' || digit == 'C') return 12;
    else if (digit == 'd' || digit == 'D') return 13;
    else if (digit == 'e' || digit == 'E') return 14;
    else if (digit == 'f' || digit == 'F') return 15;
    else {
        preprocessor_fatal_error(0, 0, 0, "invalid hex digit");
    }
}

static inline unsigned char extract_bits(unsigned int num, unsigned char i, unsigned char n) {
    const unsigned int mask = (1 << n) - 1;
    return (unsigned char)((num & (mask << i)) >> i);
}

static unsigned int eval_escape_sequence(struct str_view esc_seq, bool is_wide) {
    unsigned int max_esc_seq_val = is_wide ? ((uint64_t)1 << (sizeof(wchar_t) * 8)) - 1 : UCHAR_MAX;
    if ('0' <= esc_seq.chars[1] && esc_seq.chars[1] <= '7') {
        unsigned int out = 0;
        for (size_t i = 1; i < esc_seq.n; i++) {
            out += (unsigned int)((esc_seq.chars[i] - '0') << ((esc_seq.n - 1 - i) * 3));
        }
        if (out > max_esc_seq_val) {
            preprocessor_fatal_error(0, 0, 0, "octal escape sequence represents a number too large");
        }
        return out;
    } else if (esc_seq.chars[1] == 'x') {
        unsigned int out = 0;
        for (size_t i = 2; i < esc_seq.n; i++) {
            out += (unsigned int) (get_hex_digit_value(esc_seq.chars[i]) << ((esc_seq.n - 1 - i) * 4));
        }
        if (out > max_esc_seq_val) {
            preprocessor_fatal_error(0, 0, 0, "hex escape sequence represents a number too large (%x > %x)", out, max_esc_seq_val);
        }
        return out;
    } else if (esc_seq.chars[1] == 'u' || esc_seq.chars[1] == 'U') {
        unsigned int code_point = 0;
        for (size_t i = 2; i < esc_seq.n; i++) {
            code_point += (unsigned int)(get_hex_digit_value(esc_seq.chars[i]) << ((esc_seq.n - 1 - i) * 4));
        }
        if ((code_point < 0xA0u && code_point != 0x24u && code_point != 0x40u && code_point != 0x60u) || code_point > 0x10FFFFu) {
            preprocessor_fatal_error(0, 0, 0, "invalid universal character name");
        }
        if (code_point <= 0x7F || is_wide) {
            return code_point;
        } else if (code_point <= 0x7FF) {
            unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            unsigned char high_byte = 0xC0 | extract_bits(code_point, 6, 5);
            return (unsigned int)(low_byte | (high_byte << 8));
        } else if (code_point <= 0xFFFF) {
            unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            unsigned char mid_byte = 0x80 | extract_bits(code_point, 6, 6);
            unsigned char high_byte = 0xE0 | extract_bits(code_point, 12, 4);
            return (unsigned int)(low_byte | (mid_byte << 8) | (high_byte << 16));
        } else {
            unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            unsigned char mid_low_byte = 0x80 | extract_bits(code_point, 6, 6);
            unsigned char mid_high_byte = 0x80 | extract_bits(code_point, 12, 6);
            unsigned char high_byte = 0xF0 | extract_bits(code_point, 18, 3);
            return (unsigned int)(low_byte | (mid_low_byte << 8) | (mid_high_byte << 16) | (high_byte << 24));
        }
    } else if (esc_seq.chars[1] == '\'' || esc_seq.chars[1] == '\"' || esc_seq.chars[1] == '?' || esc_seq.chars[1] == '\\') {
        return get_ascii_value(esc_seq.chars[1]);
    } else {
        switch (esc_seq.chars[1]) {
            case 'a': return get_ascii_value('\a');
            case 'b': return get_ascii_value('\b');
            case 'f': return get_ascii_value('\f');
            case 'n': return get_ascii_value('\n');
            case 'r': return get_ascii_value('\r');
            case 't': return get_ascii_value('\t');
            case 'v': return get_ascii_value('\v');
            default: preprocessor_fatal_error(0, 0, 0, "invalid escape sequence");
        }
    }
}

static unsigned char get_esc_seq_n_bytes(struct str_view esc_seq) {
    if (esc_seq.chars[1] == 'U') {
        return 4;
    } else if (esc_seq.chars[1] == 'u') {
        return 2;
    } else {
        return 1;
    }
}

static ssize_t scan_esc_seq(struct str_view str, size_t i) {
    if (i + 1 >= str.n || str.chars[i] != '\\') return -1;
    i++;
    switch (str.chars[i]) {
        case '\'': case '"': case '?': case '\\':
        case 'a': case 'b': case 'f': case 'n': case 'r': case 't': case 'v':
            return (ssize_t)i+1;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
            size_t first_digit_i = i;
            while (i - first_digit_i < 3 && i < str.n && '0' <= str.chars[i] && str.chars[i] <= '7') i++;
            return (ssize_t)i;
        }
        case 'x':
            i++;
            while (i < str.n && isxdigit(str.chars[i])) i++;
            return (ssize_t)i;
        case 'u': {
            i++;
            size_t first_digit_i = i;
            while (i - first_digit_i < 4 && i < str.n && isxdigit(str.chars[i])) i++;
            return i - first_digit_i == 4 ? (ssize_t)i : -1;
        }
        case 'U': {
            i++;
            size_t first_digit_i = i;
            while (i - first_digit_i < 8 && i < str.n && isxdigit(str.chars[i])) i++;
            return i - first_digit_i == 8 ? (ssize_t)i : -1;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "tried to scan invalid escape sequence");
    }
}

static ssize_t scan_c_char(struct str_view str, size_t i) {
    if (i >= str.n || str.chars[i] == '\'') return -1;
    if (str.chars[i] == '\\') {
        return scan_esc_seq(str, i);
    } else if (in_src_char_set(str.chars[i])) {
        return (ssize_t)i+1;
    } else {
        return -1;
    }
}

struct parsed_char_constant {
    struct str_view *c_chars;
    size_t n_c_chars;
    bool is_wide;
};

static struct parsed_char_constant parse_char_constant(struct str_view char_constant) {
    bool is_wide;
    ssize_t i;
    if (char_constant.chars[0] == 'L') {
        is_wide = true;
        i = 2;
    } else {
        is_wide = false;
        i = 1;
    }
    str_view_vec c_chars;
    str_view_vec_init(&c_chars, 0);
    while (true) {
        size_t char_start = (size_t)i;
        i = scan_c_char(char_constant, (size_t)i);
        if (i == -1) break;
        else {
            str_view_vec_append(&c_chars, (struct str_view) {
                .chars = char_constant.chars + char_start, .n = (size_t)i - char_start
            });
        }
    }
    return (struct parsed_char_constant) {
        .c_chars = c_chars.arr, .n_c_chars = c_chars.n_elements, .is_wide = is_wide
    };
}

static int eval_char_constant(struct earley_rule rule) {
    struct str_view rule_val = rule.rhs.symbols[0].val.terminal.token.name;
    uchar_vec rule_val_vec;
    uchar_vec_init(&rule_val_vec, 0);
    uchar_vec_append_all_arr(&rule_val_vec, rule_val.chars, rule_val.n);
    struct parsed_char_constant parse = parse_char_constant((struct str_view) { .chars = rule_val_vec.arr, .n = rule_val_vec.n_elements });

    int out = 0;
    size_t current_byte = 0;
    for (ssize_t i = (ssize_t)parse.n_c_chars - 1; i >= 0; i--) {
        if (parse.c_chars[i].chars[0] != '\\') {
            unsigned char char_val = get_ascii_value(parse.c_chars[i].chars[0]);
            if (current_byte + 1 > sizeof(int)) {
                preprocessor_fatal_error(0, 0, 0, "character constant doesn't fit in an int");
            } else {
                out += char_val << (current_byte * 8);
            }
            current_byte += (parse.is_wide ? 4 : 1);
        } else {
            int char_val = (int)eval_escape_sequence(parse.c_chars[i], parse.is_wide);
            unsigned char char_n_bytes = parse.is_wide ? 4 : get_esc_seq_n_bytes(parse.c_chars[i]);
            if (current_byte + char_n_bytes > sizeof(int)) {
                preprocessor_fatal_error(0, 0, 0, "character constant doesn't fit in an int");
            } else {
                out += char_val << (current_byte * 8);
            }
            current_byte += char_n_bytes;
        }
    }
    if (!parse.is_wide && out > UCHAR_MAX) {
        preprocessor_warning(0, 0, 0, "character constant doesn't fit in an unsigned char (a byte)");
    }
    return out;
}

struct parsed_int_constant {
    struct str_view digits;
    enum int_constant_type { INT_CONSTANT_DECIMAL, INT_CONSTANT_OCTAL, INT_CONSTANT_HEX } type;
    bool is_signed;
};

static bool is_octal_digit(unsigned char c) {
    return c >= '0' && c <= '7';
}

static struct parsed_int_constant parse_int_constant(struct str_view int_constant) {
    enum int_constant_type type;
    struct str_view digits;
    size_t i;
    if (int_constant.n >= 2 && int_constant.chars[0] == '0' && (int_constant.chars[1] == 'x' || int_constant.chars[1] == 'X')) {
        type = INT_CONSTANT_HEX;
        for (i = 2; i < int_constant.n && isxdigit(int_constant.chars[i]); i++);
        digits = (struct str_view) { .chars = int_constant.chars + 2, .n = i - 2 };
    } else if (int_constant.chars[0] == '0') {
        type = INT_CONSTANT_OCTAL;
        for (i = 0; i < int_constant.n && is_octal_digit(int_constant.chars[i]); i++);
        digits = (struct str_view) { .chars = int_constant.chars, .n = i };
    } else {
        type = INT_CONSTANT_DECIMAL;
        for (i = 0; i < int_constant.n && isdigit(int_constant.chars[i]); i++);
        digits = (struct str_view) { .chars = int_constant.chars, .n = i };
    }
    bool is_signed = true;
    for (; i < int_constant.n; i++) {
        if (int_constant.chars[i] == 'u' || int_constant.chars[i] == 'U') {
            is_signed = false;
        }
    }
    return (struct parsed_int_constant) { .digits = digits, .type = type, .is_signed = is_signed };
}

static struct maybe_signed_intmax eval_int_constant(struct earley_rule rule) {
    struct str_view rule_val = rule.rhs.symbols[0].val.terminal.token.name;
    struct parsed_int_constant parse = parse_int_constant(rule_val);
    struct maybe_signed_intmax out = { .is_signed = parse.is_signed };
    if (parse.is_signed) {
        out.val.signd = 0;
        for (size_t i = 0; i < parse.digits.n; i++) {
            out.val.signd += get_hex_digit_value(parse.digits.chars[i]) << ((parse.digits.n - 1 - i) * 8);
        }
    } else {
        out.val.unsignd = 0;
        for (size_t i = 0; i < parse.digits.n; i++) {
            out.val.unsignd += (target_uintmax_t)(get_hex_digit_value(parse.digits.chars[i]) << ((parse.digits.n - 1 - i) * 8));
        }
    }
    return out;
}

static struct maybe_signed_intmax eval_constant(struct earley_rule rule) {
    switch ((enum constant_tag)rule.rhs.tag) {
        case CONSTANT_INTEGER:
            return eval_int_constant(*rule.completed_from.arr[0]);
        case CONSTANT_FLOAT:
            preprocessor_fatal_error(0, 0, 0, "preprocessor constant expressions must be integer expressions");
        case CONSTANT_ENUM:
            preprocessor_fatal_error(0, 0, 0, "enum constant should have been replaced with 0");
        case CONSTANT_CHARACTER: {
            int val = eval_char_constant(*rule.completed_from.arr[0]);
            print_with_color(TEXT_COLOR_LIGHT_RED, "char constant evaluates to 0x%X\n", val);
            return msi_s(val);
        }
    }
}

static struct maybe_signed_intmax eval_cond_expr(struct earley_rule rule);
static struct maybe_signed_intmax eval_assignment_expr(struct earley_rule rule) {
    switch ((enum assignment_expr_tag)rule.rhs.tag) {
        case ASSIGNMENT_EXPR_CONDITIONAL:
            return eval_cond_expr(*rule.completed_from.arr[0]);
        case ASSIGNMENT_EXPR_NORMAL:
            preprocessor_fatal_error(0, 0, 0, "assignment not allowed in preprocessor constant expression");
    }
}

static struct maybe_signed_intmax eval_expr(struct earley_rule rule) {
    switch((enum list_rule_tag)rule.rhs.tag) {
        case LIST_RULE_ONE:
            return eval_assignment_expr(*rule.completed_from.arr[0]);
        case LIST_RULE_MULTI:
            preprocessor_fatal_error(0, 0, 0, "commas not allowed in preprocessor constant expression");
    }
}

static struct maybe_signed_intmax eval_primary_expr(struct earley_rule rule) {
    switch ((enum primary_expr_tag)rule.rhs.tag) {
        case PRIMARY_EXPR_IDENTIFIER:
            preprocessor_fatal_error(0, 0, 0, "identifier should have been replaced with 0");
        case PRIMARY_EXPR_CONSTANT:
            return eval_constant(*rule.completed_from.arr[0]);
        case PRIMARY_EXPR_STRING:
            preprocessor_fatal_error(0, 0, 0, "string literals aren't allowed in a preprocessor constant expression");
        case PRIMARY_EXPR_PARENS:
            return eval_expr(*rule.completed_from.arr[0]);
    }
}

static struct maybe_signed_intmax eval_postfix_expr(struct earley_rule rule) {
    switch ((enum postfix_expr_tag)rule.rhs.tag) {
        case POSTFIX_EXPR_PRIMARY:
            return eval_primary_expr(*rule.completed_from.arr[0]);
        case POSTFIX_EXPR_ARRAY_ACCESS:
        case POSTFIX_EXPR_FUNC:
        case POSTFIX_EXPR_DOT:
        case POSTFIX_EXPR_ARROW:
        case POSTFIX_EXPR_INC:
        case POSTFIX_EXPR_DEC:
        case POSTFIX_EXPR_COMPOUND_LITERAL:
            preprocessor_fatal_error(0, 0, 0, "operation not supported in preprocessor constant expression");
    }
}

static struct maybe_signed_intmax eval_cast_expr(struct earley_rule rule);
static struct maybe_signed_intmax eval_unary_expr(struct earley_rule rule) {
    switch((enum unary_expr_tag)rule.rhs.tag) {
        case UNARY_EXPR_POSTFIX:
            return eval_postfix_expr(*rule.completed_from.arr[0]);
        case UNARY_EXPR_INC: case UNARY_EXPR_DEC:
            preprocessor_fatal_error(0, 0, 0, "++ and -- aren't allowed in constant expressions");
        case UNARY_EXPR_UNARY_OP: {
            struct maybe_signed_intmax expr_val = eval_cast_expr(*rule.completed_from.arr[1]);
            switch (rule.completed_from.arr[0]->rhs.tag) {
                case UNARY_OPERATOR_PLUS:
                    if (expr_val.is_signed) return msi_s(+expr_val.val.signd);
                    else return msi_u(+expr_val.val.unsignd);
                case UNARY_OPERATOR_MINUS:
                    if (expr_val.is_signed) return msi_s(-expr_val.val.signd);
                    else return msi_u(-expr_val.val.unsignd);
                case UNARY_OPERATOR_BITWISE_NOT:
                    if (expr_val.is_signed) return msi_s(~expr_val.val.signd);
                    else return msi_u(~expr_val.val.unsignd);
                case UNARY_OPERATOR_LOGICAL_NOT:
                    if (expr_val.is_signed) return msi_s(!expr_val.val.signd);
                    else return msi_s(!expr_val.val.unsignd); // not a typo
                case UNARY_OPERATOR_DEREFERENCE:
                    preprocessor_fatal_error(0, 0, 0, "dereference operator isn't allowed in constant expressions");
                case UNARY_OPERATOR_ADDRESS_OF:
                    preprocessor_fatal_error(0, 0, 0, "address-of operator isn't allowed in constant expressions");
            }
        }
        case UNARY_EXPR_SIZEOF_UNARY: case UNARY_EXPR_SIZEOF_TYPE:
            preprocessor_fatal_error(0, 0, 0,
                                     "sizeof found in preprocessor constant expression; should have been replaced with 0 earlier");
    }
}

static struct maybe_signed_intmax eval_cast_expr(struct earley_rule rule) {
    switch ((enum cast_expr_tag)rule.rhs.tag) {
        case CAST_EXPR_UNARY:
            return eval_unary_expr(*rule.completed_from.arr[0]);
        case CAST_EXPR_NORMAL:
            preprocessor_fatal_error(0, 0, 0, "cast expressions aren't allowed in constant expressions");
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
EVAL_3_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_mult_expr, eval_cast_expr, *, /, %)
EVAL_2_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_add_expr, eval_mult_expr, +, -)
EVAL_2_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_shift_expr, eval_add_expr, <<, >>)
EVAL_4_ALT_BINARY_OP_SIGNED_RESULT(eval_rel_expr, eval_shift_expr, <, >, <=, >=)
EVAL_2_ALT_BINARY_OP_SIGNED_RESULT(eval_eq_expr, eval_rel_expr, ==, !=)
EVAL_1_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_and_expr, eval_eq_expr, &)
EVAL_1_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_eor_expr, eval_and_expr, ^)
EVAL_1_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_ior_expr, eval_eor_expr, |)
EVAL_1_ALT_BINARY_OP_SIGNED_RESULT(eval_land_expr, eval_ior_expr, &&)
EVAL_1_ALT_BINARY_OP_USUAL_CONVERSIONS(eval_lor_expr, eval_land_expr, ||)
#pragma clang diagnostic pop

static bool msi_is_nonzero(struct maybe_signed_intmax msi) {
    return (msi.is_signed && msi.val.signd != 0) || (!msi.is_signed && msi.val.unsignd != 0);
}

static struct maybe_signed_intmax eval_cond_expr(struct earley_rule rule) {
    switch ((enum cond_expr_tag)rule.rhs.tag) {
        case COND_EXPR_LOGICAL_OR:
            return eval_lor_expr(*rule.completed_from.arr[0]);
        case COND_EXPR_NORMAL: {
            struct maybe_signed_intmax condition_result = eval_lor_expr(*rule.completed_from.arr[0]);
            if (msi_is_nonzero(condition_result)) {
                return eval_expr(*rule.completed_from.arr[1]);
            } else {
                return eval_cond_expr(*rule.completed_from.arr[2]);
            }
        }
    }
}

static struct maybe_signed_intmax eval_int_const_expr(struct earley_rule constant_expression_rule) {
    struct maybe_signed_intmax out = eval_cond_expr(*constant_expression_rule.completed_from.arr[0]);
    if (out.is_signed) {
        print_with_color(TEXT_COLOR_LIGHT_RED, "constant expression evaluates to %jd\n", out.val.signd);
    } else {
        print_with_color(TEXT_COLOR_LIGHT_RED, "constant expression evaluates to %ju\n", out.val.unsignd);
    }
    return out;
}

struct earley_rule *eval_if_section(struct earley_rule if_section_rule) {
    struct earley_rule if_group_rule = *if_section_rule.completed_from.arr[0];
    struct earley_rule expr_rule = *if_group_rule.completed_from.arr[0];
    struct maybe_signed_intmax expr_val = eval_int_const_expr(expr_rule);
    if (msi_is_nonzero(expr_val)) {
        struct earley_rule *group_opt_rule = if_group_rule.completed_from.arr[1];
        return group_opt_rule;
    } else {
        return NULL;
    }
}
