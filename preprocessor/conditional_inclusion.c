#include <math.h>
#include <limits.h>
#include <inttypes.h>

#include "conditional_inclusion.h"
#include "preprocessor/diagnostics.h"
#include "debug/color_print.h"
#include "mappings/typedefs.h"

static struct maybe_signed_intmax msi_s(const target_intmax_t num) {
    return (struct maybe_signed_intmax) { .is_signed = true, .val.signd = num };
}

static struct maybe_signed_intmax msi_u(const target_uintmax_t num) {
    return (struct maybe_signed_intmax) { .is_signed = false, .val.unsignd = num };
}

#define MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(msi1, msi2, op) \
do {                                                                \
if (!(msi1).is_signed && !(msi2).is_signed) {                       \
    return msi_u((msi1).val.unsignd op (msi2).val.unsignd);         \
} else if ((msi1).is_signed && !(msi2).is_signed) {                 \
    return msi_u((msi1).val.signd op (msi2).val.unsignd);           \
} else if (!(msi1).is_signed && (msi2).is_signed) {                 \
    return msi_u((msi1).val.unsignd op (msi2).val.signd);           \
} else {                                                            \
    return msi_s((msi1).val.signd op (msi2).val.signd);             \
}                                                                   \
} while (0)

#define MSI_BINARY_OP_RETURN_SIGNED_RESULT(msi1, msi2, op)  \
do {                                                        \
if (!(msi1).is_signed && !(msi2).is_signed) {               \
    return msi_s((msi1).val.unsignd op (msi2).val.unsignd); \
} else if ((msi1).is_signed && !(msi2).is_signed) {         \
    return msi_s((msi1).val.signd op (msi2).val.unsignd);   \
} else if (!(msi1).is_signed && (msi2).is_signed) {         \
    return msi_s((msi1).val.unsignd op (msi2).val.signd);   \
} else {                                                    \
    return msi_s((msi1).val.signd op (msi2).val.signd);     \
}                                                           \
} while (0)


static unsigned char get_ascii_value(const unsigned char c) {
    // TODO hardcode all possible values so this doesn't depend on the compiler
    return c;
}

static unsigned char get_hex_digit_value(const unsigned char digit) {
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

static unsigned char extract_bits(const unsigned int num, const unsigned char i, const unsigned char n) {
    const unsigned int mask = (1 << n) - 1;
    return (unsigned char)((num & (mask << i)) >> i);
}

static unsigned int eval_escape_sequence(const sstr esc_seq, const bool is_wide) {
    const unsigned int max_esc_seq_val = is_wide ? ((uint64_t)1 << (sizeof(wchar_t) * 8)) - 1 : UCHAR_MAX;
    if ('0' <= esc_seq.data[1] && esc_seq.data[1] <= '7') {
        unsigned int out = 0;
        for (size_t i = 1; i < esc_seq.len; i++) {
            out += (unsigned int) ((esc_seq.data[i] - '0') << ((esc_seq.len - 1 - i) * 3));
        }
        if (out > max_esc_seq_val) {
            preprocessor_fatal_error(0, 0, 0, "octal escape sequence represents a number too large");
        }
        return out;
    } else if (esc_seq.data[1] == 'x') {
        unsigned int out = 0;
        for (size_t i = 2; i < esc_seq.len; i++) {
            out += (unsigned int) (get_hex_digit_value(esc_seq.data[i]) << ((esc_seq.len - 1 - i) * 4));
        }
        if (out > max_esc_seq_val) {
            preprocessor_fatal_error(0, 0, 0, "hex escape sequence represents a number too large (%x > %x)", out, max_esc_seq_val);
        }
        return out;
    } else if (esc_seq.data[1] == 'u' || esc_seq.data[1] == 'U') {
        unsigned int code_point = 0;
        for (size_t i = 2; i < esc_seq.len; i++) {
            code_point += (unsigned int) (get_hex_digit_value(esc_seq.data[i]) << ((esc_seq.len - 1 - i) * 4));
        }
        if ((code_point < 0xA0u && code_point != 0x24u && code_point != 0x40u && code_point != 0x60u) || code_point > 0x10FFFFu) {
            preprocessor_fatal_error(0, 0, 0, "invalid universal character name");
        }
        if (code_point <= 0x7F || is_wide) {
            return code_point;
        } else if (code_point <= 0x7FF) {
            const unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            const unsigned char high_byte = 0xC0 | extract_bits(code_point, 6, 5);
            return (unsigned int)(low_byte | ((unsigned int)high_byte << 8));
        } else if (code_point <= 0xFFFF) {
            const unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            const unsigned char mid_byte = 0x80 | extract_bits(code_point, 6, 6);
            const unsigned char high_byte = 0xE0 | extract_bits(code_point, 12, 4);
            return (unsigned int)(low_byte | ((unsigned int)mid_byte << 8) | ((unsigned int)high_byte << 16));
        } else {
            const unsigned char low_byte = 0x80 | extract_bits(code_point, 0, 6);
            const unsigned char mid_low_byte = 0x80 | extract_bits(code_point, 6, 6);
            const unsigned char mid_high_byte = 0x80 | extract_bits(code_point, 12, 6);
            const unsigned char high_byte = 0xF0 | extract_bits(code_point, 18, 3);
            return (unsigned int)(low_byte | ((unsigned int)mid_low_byte << 8) | ((unsigned int)mid_high_byte << 16) | ((unsigned int)high_byte << 24));
        }
    } else if (esc_seq.data[1] == '\'' || esc_seq.data[1] == '\"' || esc_seq.data[1] == '?' || esc_seq.data[1] == '\\') {
        return get_ascii_value(esc_seq.data[1]);
    } else {
        switch (esc_seq.data[1]) {
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

static unsigned char get_esc_seq_n_bytes(const sstr esc_seq) {
    if (esc_seq.data[1] == 'U') {
        return 4;
    } else if (esc_seq.data[1] == 'u') {
        return 2;
    } else {
        return 1;
    }
}

static ssize_t scan_esc_seq(const sstr str, size_t i) {
    if (i + 1 >= str.len || str.data[i] != '\\') return -1;
    i++;
    switch (str.data[i]) {
        case '\'': case '"': case '?': case '\\':
        case 'a': case 'b': case 'f': case 'n': case 'r': case 't': case 'v':
            return (ssize_t)i+1;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
            const size_t first_digit_i = i;
            while (i - first_digit_i < 3 && i < str.len && '0' <= str.data[i] && str.data[i] <= '7') i++;
            return (ssize_t)i;
        }
        case 'x':
            i++;
            while (i < str.len && isxdigit(str.data[i])) i++;
            return (ssize_t)i;
        case 'u': {
            i++;
            const size_t first_digit_i = i;
            while (i - first_digit_i < 4 && i < str.len && isxdigit(str.data[i])) i++;
            return i - first_digit_i == 4 ? (ssize_t)i : -1;
        }
        case 'U': {
            i++;
            const size_t first_digit_i = i;
            while (i - first_digit_i < 8 && i < str.len && isxdigit(str.data[i])) i++;
            return i - first_digit_i == 8 ? (ssize_t)i : -1;
        }
        default:
            preprocessor_fatal_error(0, 0, 0, "tried to scan invalid escape sequence");
    }
}

static ssize_t scan_c_char(const sstr str, size_t i) {
    if (i >= str.len || str.data[i] == '\'') return -1;
    if (str.data[i] == '\\') {
        return scan_esc_seq(str, i);
    } else if (in_src_char_set(str.data[i])) {
        return (ssize_t)i+1;
    } else {
        return -1;
    }
}

struct parsed_char_constant {
    sstr_harr c_chars;
    bool is_wide;
};

static struct parsed_char_constant parse_char_constant(const sstr char_constant) {
    bool is_wide;
    ssize_t i;
    if (char_constant.data[0] == 'L') {
        is_wide = true;
        i = 2;
    } else {
        is_wide = false;
        i = 1;
    }
    sstr_vec c_chars = sstr_vec_new(0);
    while (true) {
        const size_t char_start = (size_t)i;
        i = scan_c_char(char_constant, (size_t)i);
        if (i == -1) break;
        else {
            sstr_vec_append(&c_chars, slice(char_constant, char_start, (size_t)i));
        }
    }
    return (struct parsed_char_constant) {
        .c_chars = c_chars.arr, .is_wide = is_wide
    };
}

static int eval_char_constant(const struct earley_rule rule) {
    const sstr rule_val = rule.rhs.symbols.data[0].val.terminal.token.name;
    uchar_vec rule_val_vec = uchar_vec_new(0);
    uchar_vec_append_all_arr(&rule_val_vec, rule_val.data, rule_val.len);
    const struct parsed_char_constant parse = parse_char_constant(rule_val_vec.arr);

    int out = 0;
    size_t current_byte = 0;
    for (ssize_t i = (ssize_t)parse.c_chars.len - 1; i >= 0; i--) {
        if (parse.c_chars.data[i].data[0] != '\\') {
            const unsigned char char_val = get_ascii_value(parse.c_chars.data[i].data[0]);
            if (current_byte + 1 > sizeof(int)) {
                preprocessor_fatal_error(0, 0, 0, "character constant doesn't fit in an int");
            } else {
                out += char_val << (current_byte * 8);
            }
            current_byte += (parse.is_wide ? 4 : 1);
        } else {
            const int char_val = (int)eval_escape_sequence(parse.c_chars.data[i], parse.is_wide);
            const unsigned char char_n_bytes = parse.is_wide ? 4 : get_esc_seq_n_bytes(parse.c_chars.data[i]);
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
    sstr digits;
    enum int_constant_type { INT_CONSTANT_DECIMAL, INT_CONSTANT_OCTAL, INT_CONSTANT_HEX } type;
    bool is_signed;
};

static bool is_octal_digit(const unsigned char c) {
    return c >= '0' && c <= '7';
}

static struct parsed_int_constant parse_int_constant(const sstr int_constant) {
    enum int_constant_type type;
    sstr digits;
    size_t i;
    if (int_constant.len >= 2 && int_constant.data[0] == '0' && (int_constant.data[1] == 'x' || int_constant.data[1] == 'X')) {
        type = INT_CONSTANT_HEX;
        for (i = 2; i < int_constant.len && isxdigit(int_constant.data[i]); i++);
        digits = slice(int_constant, 2, i);
    } else if (int_constant.data[0] == '0') {
        type = INT_CONSTANT_OCTAL;
        for (i = 0; i < int_constant.len && is_octal_digit(int_constant.data[i]); i++);
        digits = slice(int_constant, 0, i);
    } else {
        type = INT_CONSTANT_DECIMAL;
        for (i = 0; i < int_constant.len && isdigit(int_constant.data[i]); i++);
        digits = slice(int_constant, 0, i);
    }
    bool is_signed = true;
    for (; i < int_constant.len; i++) {
        if (int_constant.data[i] == 'u' || int_constant.data[i] == 'U') {
            is_signed = false;
        }
    }
    return (struct parsed_int_constant) { .digits = digits, .type = type, .is_signed = is_signed };
}

static target_uintmax_t impow(const target_uintmax_t base, const target_uintmax_t exp) {
    target_uintmax_t out = 1;
    for (target_uintmax_t i = 0; i < exp; i++) {
        out *= base;
    }
    return out;
}

static struct maybe_signed_intmax eval_int_constant(const struct earley_rule rule) {
    const sstr rule_val = rule.rhs.symbols.data[0].val.terminal.token.name;
    const struct parsed_int_constant parse = parse_int_constant(rule_val);
    target_uintmax_t result = 0;
    target_uintmax_t base =
        parse.type == INT_CONSTANT_OCTAL ? 8 : (
        parse.type == INT_CONSTANT_DECIMAL ? 10
        : 16 // INT_CONSTANT_HEX
        );
    for (size_t i = 0; i < parse.digits.len; i++) {
        result += (target_uintmax_t)get_hex_digit_value(parse.digits.data[i]) * impow(base, parse.digits.len - i - 1);
    }
    print_with_color(TEXT_COLOR_LIGHT_RED, "int constant evalutes to %" PRIuMAX "\n", result);
    if (!parse.is_signed || result > impow(2, sizeof(target_intmax_t)*8 - 1) - 1) {
        return (struct maybe_signed_intmax) { .is_signed = true, .val.unsignd = result };
    } else {
        return (struct maybe_signed_intmax) { .is_signed = false, .val.signd = (target_intmax_t)result };
    }
}

static struct maybe_signed_intmax eval_constant(const struct earley_rule rule) {
    switch ((enum constant_tag)rule.rhs.tag) {
        case CONSTANT_INTEGER:
            return eval_int_constant(*rule.completed_from.data[0]);
        case CONSTANT_FLOAT:
            preprocessor_fatal_error(0, 0, 0, "preprocessor constant expressions must be integer expressions");
        case CONSTANT_ENUM:
            preprocessor_fatal_error(0, 0, 0, "enum constant should have been replaced with 0");
        case CONSTANT_CHARACTER: {
            const int val = eval_char_constant(*rule.completed_from.data[0]);
            print_with_color(TEXT_COLOR_LIGHT_RED, "char constant evaluates to 0x%X\n", val);
            return msi_s(val);
        }
    }
}

static struct maybe_signed_intmax eval_cond_expr(struct earley_rule rule);
static struct maybe_signed_intmax eval_assignment_expr(const struct earley_rule rule) {
    switch ((enum assignment_expr_tag)rule.rhs.tag) {
        case ASSIGNMENT_EXPR_CONDITIONAL:
            return eval_cond_expr(*rule.completed_from.data[0]);
        case ASSIGNMENT_EXPR_NORMAL:
            preprocessor_fatal_error(0, 0, 0, "assignment not allowed in preprocessor constant expression");
    }
}

static struct maybe_signed_intmax eval_expr(const struct earley_rule rule) {
    switch((enum list_rule_tag)rule.rhs.tag) {
        case LIST_RULE_ONE:
            return eval_assignment_expr(*rule.completed_from.data[0]);
        case LIST_RULE_MULTI:
            preprocessor_fatal_error(0, 0, 0, "commas not allowed in preprocessor constant expression");
    }
}

static struct maybe_signed_intmax eval_primary_expr(const struct earley_rule rule) {
    switch ((enum primary_expr_tag)rule.rhs.tag) {
        case PRIMARY_EXPR_IDENTIFIER:
            preprocessor_fatal_error(0, 0, 0, "identifier should have been replaced with 0");
        case PRIMARY_EXPR_CONSTANT:
            return eval_constant(*rule.completed_from.data[0]);
        case PRIMARY_EXPR_STRING:
            preprocessor_fatal_error(0, 0, 0, "string literals aren't allowed in a preprocessor constant expression");
        case PRIMARY_EXPR_PARENS:
            return eval_expr(*rule.completed_from.data[0]);
    }
}

static struct maybe_signed_intmax eval_postfix_expr(const struct earley_rule rule) {
    switch ((enum postfix_expr_tag)rule.rhs.tag) {
        case POSTFIX_EXPR_PRIMARY:
            return eval_primary_expr(*rule.completed_from.data[0]);
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
static struct maybe_signed_intmax eval_unary_expr(const struct earley_rule rule) {
    switch((enum unary_expr_tag)rule.rhs.tag) {
        case UNARY_EXPR_POSTFIX:
            return eval_postfix_expr(*rule.completed_from.data[0]);
        case UNARY_EXPR_INC: case UNARY_EXPR_DEC:
            preprocessor_fatal_error(0, 0, 0, "++ and -- aren't allowed in constant expressions");
        case UNARY_EXPR_UNARY_OP: {
            const struct maybe_signed_intmax expr_val = eval_cast_expr(*rule.completed_from.data[1]);
            switch ((enum unary_operator_tag)rule.completed_from.data[0]->rhs.tag) {
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

static struct maybe_signed_intmax eval_cast_expr(const struct earley_rule rule) {
    switch ((enum cast_expr_tag)rule.rhs.tag) {
        case CAST_EXPR_UNARY:
            return eval_unary_expr(*rule.completed_from.data[0]);
        case CAST_EXPR_NORMAL:
            preprocessor_fatal_error(0, 0, 0, "cast expressions aren't allowed in constant expressions");
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
static struct maybe_signed_intmax eval_mult_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == MULTIPLICATIVE_EXPR_CAST) {
        return eval_cast_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == MULTIPLICATIVE_EXPR_MULT) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_mult_expr(*rule.completed_from.data[0]), eval_cast_expr(*rule.completed_from.data[1]), *);
    } else if (rule.rhs.tag == MULTIPLICATIVE_EXPR_DIV) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_mult_expr(*rule.completed_from.data[0]), eval_cast_expr(*rule.completed_from.data[1]), /);
    } else if (rule.rhs.tag == MULTIPLICATIVE_EXPR_MOD) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_mult_expr(*rule.completed_from.data[0]), eval_cast_expr(*rule.completed_from.data[1]), %);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Multiplicative expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_add_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == ADDITIVE_EXPR_MULT) {
        return eval_mult_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == ADDITIVE_EXPR_PLUS) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_add_expr(*rule.completed_from.data[0]), eval_mult_expr(*rule.completed_from.data[1]), +);
    } else if (rule.rhs.tag == ADDITIVE_EXPR_MINUS) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_add_expr(*rule.completed_from.data[0]), eval_mult_expr(*rule.completed_from.data[1]), -);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Additive expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_shift_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == SHIFT_EXPR_ADDITIVE) {
        return eval_add_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == SHIFT_EXPR_LEFT) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_shift_expr(*rule.completed_from.data[0]), eval_add_expr(*rule.completed_from.data[1]), <<);
    } else if (rule.rhs.tag == SHIFT_EXPR_RIGHT) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_shift_expr(*rule.completed_from.data[0]), eval_add_expr(*rule.completed_from.data[1]), >>);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Shift expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_rel_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == RELATIONAL_EXPR_SHIFT) {
        return eval_shift_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == RELATIONAL_EXPR_LESS) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_rel_expr(*rule.completed_from.data[0]), eval_shift_expr(*rule.completed_from.data[1]), <);
    } else if (rule.rhs.tag == RELATIONAL_EXPR_GREATER) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_rel_expr(*rule.completed_from.data[0]), eval_shift_expr(*rule.completed_from.data[1]), >);
    } else if (rule.rhs.tag == RELATIONAL_EXPR_LEQ) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_rel_expr(*rule.completed_from.data[0]), eval_shift_expr(*rule.completed_from.data[1]), <=);
    } else if (rule.rhs.tag == RELATIONAL_EXPR_GEQ) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_rel_expr(*rule.completed_from.data[0]), eval_shift_expr(*rule.completed_from.data[1]), >=);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Relational expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_eq_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == EQUALITY_EXPR_RELATIONAL) {
        return eval_rel_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == EQUALITY_EXPR_EQUAL) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_eq_expr(*rule.completed_from.data[0]), eval_rel_expr(*rule.completed_from.data[1]), ==);
    } else if (rule.rhs.tag == EQUALITY_EXPR_NOT_EQUAL) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_eq_expr(*rule.completed_from.data[0]), eval_rel_expr(*rule.completed_from.data[1]), !=);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Equality expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_and_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == AND_EXPR_EQUALITY) {
        return eval_eq_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == AND_EXPR_NORMAL) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_and_expr(*rule.completed_from.data[0]), eval_eq_expr(*rule.completed_from.data[1]), &);
    } else {
        preprocessor_fatal_error(0, 0, 0, "And expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_eor_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == EXCLUSIVE_OR_EXPR_AND) {
        return eval_and_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == EXCLUSIVE_OR_EXPR_NORMAL) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_eor_expr(*rule.completed_from.data[0]), eval_and_expr(*rule.completed_from.data[1]), ^);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Exclusive or expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_ior_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == INCLUSIVE_OR_EXPR_EXCLUSIVE_OR) {
        return eval_eor_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == INCLUSIVE_OR_EXPR_NORMAL) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_ior_expr(*rule.completed_from.data[0]), eval_eor_expr(*rule.completed_from.data[1]), |);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Inclusive or expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_land_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == LOGICAL_AND_EXPR_INCLUSIVE_OR) {
        return eval_ior_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == LOGICAL_AND_EXPR_NORMAL) {
        MSI_BINARY_OP_RETURN_SIGNED_RESULT(eval_land_expr(*rule.completed_from.data[0]), eval_ior_expr(*rule.completed_from.data[1]), &&);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Logical and expression tag is not recognized");
    }
}

static struct maybe_signed_intmax eval_lor_expr(const struct earley_rule rule) {
    if (rule.rhs.tag == LOGICAL_OR_EXPR_LOGICAL_AND) {
        return eval_land_expr(*rule.completed_from.data[0]);
    } else if (rule.rhs.tag == LOGICAL_OR_EXPR_NORMAL) {
        MSI_BINARY_OP_RETURN_WITH_USUAL_CONVERSIONS(eval_lor_expr(*rule.completed_from.data[0]), eval_land_expr(*rule.completed_from.data[1]), ||);
    } else {
        preprocessor_fatal_error(0, 0, 0, "Logical or expression tag is not recognized");
    }
}
#pragma clang diagnostic pop

static bool msi_is_nonzero(const struct maybe_signed_intmax msi) {
    return (msi.is_signed && msi.val.signd != 0) || (!msi.is_signed && msi.val.unsignd != 0);
}

static struct maybe_signed_intmax eval_cond_expr(const struct earley_rule rule) {
    switch ((enum cond_expr_tag)rule.rhs.tag) {
        case COND_EXPR_LOGICAL_OR:
            return eval_lor_expr(*rule.completed_from.data[0]);
        case COND_EXPR_NORMAL: {
            const struct maybe_signed_intmax condition_result = eval_lor_expr(*rule.completed_from.data[0]);
            if (msi_is_nonzero(condition_result)) {
                return eval_expr(*rule.completed_from.data[1]);
            } else {
                return eval_cond_expr(*rule.completed_from.data[2]);
            }
        }
    }
}

static struct maybe_signed_intmax eval_int_const_expr(const struct earley_rule constant_expression_rule) {
    return eval_cond_expr(*constant_expression_rule.completed_from.data[0]);
}

struct earley_rule *eval_if_section(const struct earley_rule if_section_rule) {
    const struct earley_rule if_group_rule = *if_section_rule.completed_from.data[0];
    const struct earley_rule expr_rule = *if_group_rule.completed_from.data[0];
    const struct maybe_signed_intmax expr_val = eval_int_const_expr(expr_rule);
    if (msi_is_nonzero(expr_val)) {
        struct earley_rule *group_opt_rule = if_group_rule.completed_from.data[1];
        return group_opt_rule;
    } else {
        return NULL;
    }
}
