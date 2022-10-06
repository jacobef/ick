#ifndef ICK_PP_DIRECTIVE_H
#define ICK_PP_DIRECTIVE_H

#include <stdbool.h>
#include "preprocessor/pp_token.h"


struct decimal_constant_detector {
    enum detection_status status;
};

struct octal_constant_detector {
    enum detection_status status;
};

struct hex_constant_detector {
    enum detection_status status;
    bool is_first_char;
    bool looking_for_xX;
    bool looking_for_digit;
};

struct hex_constant_detector detect_hex_constant(struct hex_constant_detector detector, char c);

struct integer_suffix_detector {
    enum detection_status status;
    char chars[4];
    unsigned char n_chars;
};

struct integer_suffix_detector detect_integer_suffix(struct integer_suffix_detector detector, char c);

struct integer_constant_detector {
    enum detection_status status;
    enum detection_status prev_status;
    struct decimal_constant_detector decimal_detector;
    struct octal_constant_detector octal_detector;
    struct hex_constant_detector hex_detector;
    struct integer_suffix_detector suffix_detector;
    bool looking_for_suffix;
};

struct integer_constant_detector detect_integer_constant(struct integer_constant_detector detector, char c);

#endif //ICK_PP_DIRECTIVE_H
