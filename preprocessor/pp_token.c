#include <ctype.h>
#include "pp_token.h"

static bool is_letter(char c) {
    switch (c) {
        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':
        case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':
        case 'Y':case 'Z':
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':
        case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':
        case 'y':case 'z':
            return true;
        default:
            return false;
    }
}

static bool is_hex_digit(char c) {
    if (c >= '0' && c <= '9') return true;
    switch (c) {
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
            return true;
        default:
            return false;
    }
}

static bool is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

static bool in_src_char_set(char c) {
    if (is_hex_digit(c) || is_letter(c)) return true;
    switch (c) {
        case '!':case '"':case '#':case '%':case '&':case '\'':case '(':case ')':case '*':case '+':case ',':case '-':
        case '.':case '/':case ':':case ';':case '<':case '=':case '>':case '?':case '[':case '\\':case ']':case '^':
        case '_':case '{':case '|':case '}':case '~':
        case ' ':case '\t':case '\v':case '\f':
            return true;
        default:
            return false;
    }
}

struct header_name_detector detect_header_name(struct header_name_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (c == '"' && detector.is_first_char) {
        detector.in_quotes = true;
    }
    else if (c == '<' && detector.is_first_char) {
        detector.in_quotes = false;
    }
    else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    }
    else if ((c == '"' && detector.in_quotes) || (c == '>' && !detector.in_quotes)) {
        detector.status = TRUE;
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

struct universal_character_name_detector detect_universal_character_name(struct universal_character_name_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.is_first_char && c == '\\') {
        detector.looking_for_uU = true;
    }
    else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.looking_for_uU && c == 'u') {
        detector.looking_for_uU = false;
        detector.looking_for_digits = true;
        detector.expected_digits = 4;
    }
    else if (detector.looking_for_uU && c == 'U') {
        detector.looking_for_uU = false;
        detector.looking_for_digits = true;
        detector.expected_digits = 8;
    }
    else if (detector.looking_for_uU) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.looking_for_digits && is_hex_digit(c)) {
        detector.n_digits++;
        if (detector.n_digits == detector.expected_digits) {
            detector.status = TRUE;
            detector.looking_for_uU = false;
            detector.looking_for_digits = false;
            detector.is_first_char = true;
            detector.n_digits = 0;
        }
    }
    else if (detector.looking_for_digits) {
        detector.status = IMPOSSIBLE;
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

struct identifier_detector detect_identifier(struct identifier_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (!in_src_char_set(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char && isdigit(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == '\\') {
        detector.looking_for_ucn = true;
    }

    if (detector.looking_for_ucn) {
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
        if (detector.ucn_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
        }
        else if (detector.ucn_detector.status == TRUE) {
            detector.looking_for_ucn = false;
        }
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

struct pp_number_detector detect_pp_number(struct pp_number_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.is_first_char && isdigit(c)) {
        detector.status = TRUE;
    }
    else if (detector.is_first_char && c == '.') {
        detector.looking_for_digit = true;
        detector.status = POSSIBLE;
    }
    else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.looking_for_digit && isdigit(c)) {
        detector.looking_for_digit = false;
        detector.status = TRUE;
    }
    else if (detector.looking_for_sign && (c == '+' || c == '-')) {
        detector.looking_for_sign = false;
        detector.status = TRUE;
    }
    else if (detector.status == TRUE) {
        if (c == 'e' || c == 'E' || c == 'p' || c == 'P') {
            detector.looking_for_sign = true;
            detector.status = POSSIBLE;
        }
        else if (c == '\\') {
            detector.looking_for_ucn = true;
            detector.status = POSSIBLE;
        }
        else if (!(is_letter(c) || c == '_' || isdigit(c) || c == '.'))  {
            detector.status = IMPOSSIBLE;
        }
    }

    if (detector.looking_for_ucn) {
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
        if (detector.ucn_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
        }
        else if (detector.ucn_detector.status == TRUE) {
            detector.looking_for_ucn = false;
            detector.status = TRUE;
        }
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

struct escape_sequence_detector detect_escape_sequence(struct escape_sequence_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.next_char_invalid) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char && c == '\\') {
    }
    else if (detector.is_second_char && (c == '\'' || c == '"' || c == '?' || c == '\\' ||
                c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v')) {
        detector.status = TRUE;
        detector.next_char_invalid = true;
    }
    else if (detector.is_second_char && c == 'x') {
        detector.looking_for_hex = true;
    }
    else if (detector.is_second_char && (c == 'u' || c == 'U')) {
        detector.looking_for_ucn = true;
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, '\\');
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
    }
    else if (detector.looking_for_ucn) {
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
        if (detector.ucn_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
        }
        else if (detector.ucn_detector.status == TRUE) {
            detector.status = TRUE;
            detector.looking_for_ucn = false;
        }
    }
    else if (detector.is_second_char && is_octal_digit(c)) {
        detector.n_octals++;
        detector.looking_for_octal = true;
        detector.status = TRUE;
    }
    else if (detector.looking_for_octal && is_octal_digit(c)) {
        detector.n_octals++;
        if (detector.n_octals == 3) detector.next_char_invalid = true;
    }
    else if (detector.looking_for_hex && is_hex_digit(c)) {
        detector.status = TRUE;
    }
    else {
        detector.status = IMPOSSIBLE;
    }

    if (detector.is_second_char) detector.is_second_char = false;
    if (detector.is_first_char) {
        detector.is_first_char = false;
        detector.is_second_char = true;
    }

    return detector;
}

static struct char_const_str_literal_detector detect_char_const_str_literal(struct char_const_str_literal_detector detector, char quote, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.looking_for_esc_seq) {
        detector.esc_seq_detector = detect_escape_sequence(detector.esc_seq_detector, c);
        if (detector.esc_seq_detector.status == IMPOSSIBLE && detector.prev_esc_seq_status == TRUE) {
            detector.looking_for_esc_seq = false;
        }
        else if (detector.esc_seq_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
            return detector;
        }
        detector.prev_esc_seq_status = detector.esc_seq_detector.status;
    }

    if (detector.is_first_char && c == 'L') {
        detector.looking_for_open_quote = true;
    }
    else if (detector.looking_for_open_quote && c == quote) {
        detector.looking_for_char_seq = true;
        detector.looking_for_open_quote = false;
    }
    else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.looking_for_char_seq && !detector.looking_for_esc_seq && c == '\\') {
        detector.looking_for_esc_seq = true;
        detector.esc_seq_detector = initial_esc_seq_detector;
        detector.esc_seq_detector = detect_escape_sequence(detector.esc_seq_detector, c);
    }
    else if (detector.looking_for_char_seq && !detector.looking_for_esc_seq && !in_src_char_set(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote && detector.looking_for_esc_seq && detector.esc_seq_detector.status == TRUE) {
        detector.status = TRUE;
    }
    else if (c == quote && detector.looking_for_esc_seq) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote && !detector.is_first_char && detector.prev_char == quote) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote) {
        detector.status = TRUE;
    }

    if (detector.is_first_char) detector.is_first_char = false;
    detector.prev_char = c;

    return detector;
}

struct char_const_str_literal_detector detect_character_constant(struct char_const_str_literal_detector detector, char c) {
    return detect_char_const_str_literal(detector, '\'', c);
}

struct char_const_str_literal_detector detect_string_literal(struct char_const_str_literal_detector detector, char c) {
    return detect_char_const_str_literal(detector, '"', c);
}
