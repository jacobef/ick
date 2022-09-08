#ifndef ICK_PP_TOKEN_H
#define ICK_PP_TOKEN_H

#include <stdbool.h>

enum detection_status {
    IMPOSSIBLE,
    POSSIBLE,
    TRUE
};

struct header_name_detector {
    enum detection_status status;
    bool in_quotes;
    bool is_first_char;
};

struct header_name_detector detect_header_name(struct header_name_detector detector, char c);

struct universal_character_name_detector {
    enum detection_status status;
    bool looking_for_digits;
    bool looking_for_uU;
    bool is_first_char;
    unsigned char expected_digits;
    unsigned char n_digits;
};

struct universal_character_name_detector detect_universal_character_name(struct universal_character_name_detector detector, char c);

static const struct universal_character_name_detector initial_ucn_detector = {.status=POSSIBLE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false};

struct identifier_detector {
    struct universal_character_name_detector ucn_detector;
    enum detection_status status;
    bool looking_for_ucn;
    bool is_first_char;
};

struct identifier_detector detect_identifier(struct identifier_detector detector, char c);

struct pp_number_detector {
    struct universal_character_name_detector ucn_detector;
    enum detection_status status;
    bool looking_for_sign;
    bool looking_for_ucn;
    bool looking_for_digit;
    bool is_first_char;
};

struct pp_number_detector detect_pp_number(struct pp_number_detector detector, char c);

struct escape_sequence_detector {
    struct universal_character_name_detector ucn_detector;
    enum detection_status status;
    bool is_first_char;
    bool is_second_char;
    bool looking_for_ucn;
    bool looking_for_hex;
    bool looking_for_octal;
    bool next_char_invalid;
    unsigned char n_octals;
};

static const struct escape_sequence_detector initial_esc_seq_detector = {
        .status=POSSIBLE, .looking_for_hex=false, .looking_for_octal=false, .next_char_invalid=false, .n_octals=0,
        .is_first_char=true, .is_second_char=false,
        .ucn_detector={.status=POSSIBLE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false}};

struct escape_sequence_detector detect_escape_sequence(struct escape_sequence_detector detector, char c);

struct char_const_str_literal_detector {
    struct escape_sequence_detector esc_seq_detector;
    enum detection_status status;
    enum detection_status prev_esc_seq_status;
    bool looking_for_open_quote;
    bool looking_for_char_seq;
    bool looking_for_esc_seq;
    bool is_first_char;
    char prev_char; // lazy temporary solution for avoiding '' / "" and L'' / L""
};

struct char_const_str_literal_detector detect_character_constant(struct char_const_str_literal_detector detector, char c);

struct char_const_str_literal_detector detect_string_literal(struct char_const_str_literal_detector detector, char c);

struct punctuator_detector {
    enum detection_status status;
    char last_char;
};

#endif //ICK_PP_TOKEN_H
