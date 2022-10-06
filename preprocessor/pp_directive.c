#include "pp_directive.h"
#include <ctype.h>

struct decimal_constant_detector detect_decimal_constant(struct decimal_constant_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (c == '0' && detector.status != MATCH) detector.status = IMPOSSIBLE;
    else if (!isdigit(c)) detector.status = IMPOSSIBLE;
    else detector.status = MATCH;

    return detector;
}

struct octal_constant_detector detect_octal_constant(struct octal_constant_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (c != '0' && detector.status != MATCH) detector.status = IMPOSSIBLE;
    else if (!is_octal_digit(c)) detector.status = IMPOSSIBLE;
    else detector.status = MATCH;

    return detector;
}

struct hex_constant_detector detect_hex_constant(struct hex_constant_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.is_first_char && c == '0') detector.looking_for_xX = true;
    else if (detector.is_first_char) detector.status = IMPOSSIBLE;
    else if (detector.looking_for_xX && (c == 'x' || c == 'X')) {
        detector.looking_for_digit = true;
        detector.looking_for_xX = false;
    }
    else if (detector.looking_for_xX) detector.status = IMPOSSIBLE;
    else if (detector.looking_for_digit && isxdigit(c)) detector.status = MATCH;
    else if (detector.looking_for_digit) detector.status = IMPOSSIBLE;

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

struct integer_suffix_detector detect_integer_suffix(struct integer_suffix_detector detector, char c) {
    // TODO don't be lazy
    static char *matches[] = {
        "u", "U", "l", "L",
        "ll", "LL",
        "ull", "uLL", "Ull", "ULL", "ul", "uL", "Ul", "UL",
        "llu", "LLu", "llU", "LLU", "lu", "Lu", "lU", "LU"
    };
    if (detector.n_chars >= 3) {
        detector.status = IMPOSSIBLE;
        return detector;
    }
    detector.chars[detector.n_chars] = c;
    detector.n_chars++;

    for (size_t i = 0; i < sizeof(matches)/sizeof(char*); i++) {
        if (strcmp(detector.chars, matches[i]) == 0) {
            detector.status = MATCH;
            return detector;
        }
    }
    detector.status = IMPOSSIBLE;
    return detector;
}

struct integer_constant_detector detect_integer_constant(struct integer_constant_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;
    detector.prev_status = detector.status;

    detector.decimal_detector = detect_decimal_constant(detector.decimal_detector, c);
    detector.octal_detector = detect_octal_constant(detector.octal_detector, c);
    detector.hex_detector = detect_hex_constant(detector.hex_detector, c);

    // feels messy, not terrible I guess
    if (!detector.looking_for_suffix) {
        if (detector.decimal_detector.status == MATCH || detector.octal_detector.status == MATCH
            || detector.hex_detector.status == MATCH) {
            detector.status = MATCH;
        } else if (detector.decimal_detector.status == POSSIBLE || detector.octal_detector.status == POSSIBLE
                   || detector.hex_detector.status == POSSIBLE) {
            detector.status = POSSIBLE;
        } else if (detector.prev_status == MATCH) {
            detector.looking_for_suffix = true;
        } else {
            detector.status = IMPOSSIBLE;
        }
    }

    if (detector.looking_for_suffix) {
        detector.suffix_detector = detect_integer_suffix(detector.suffix_detector, c);
        detector.status = detector.suffix_detector.status;
    }

    return detector;
}

static bool token_is_str(struct preprocessing_token token, const char *str) {
    for (const char *tok_it = token.first, *str_it = str; tok_it != token.last+1 && *str_it; tok_it++, str_it++) {
        if (*tok_it != *str_it) return false;
    }
    return true;
}
