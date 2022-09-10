#include <ctype.h>
#include "pp_token.h"
#include "preprocessor/diagnostics.h"

static bool is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

static bool in_src_char_set(char c) {
    if (isdigit(c) || isalpha(c)) return true;
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
    detector.prev_status = detector.status;

    if (detector.status == TRUE) detector.status = IMPOSSIBLE;
    if (detector.status == IMPOSSIBLE) return detector;

    if (!in_src_char_set(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == '"' && detector.is_first_char) {
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
    else if (detector.looking_for_digits && isxdigit(c)) {
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
    detector.prev_status = detector.status;

    if (detector.status == IMPOSSIBLE) return detector;

    if (!(isalpha(c) || c == '_' || isdigit(c))) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char && isdigit(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == '\\') {
        detector.looking_for_ucn = true;
    }
    else if (isspace(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (!detector.looking_for_ucn) {
        detector.status = TRUE;
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

struct pp_number_detector detect_pp_number(struct pp_number_detector detector, char c) {
    detector.prev_status = detector.status;

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
    else if (detector.looking_for_digit) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.looking_for_sign && (c == '+' || c == '-')) {
        detector.looking_for_sign = false;
        detector.status = TRUE;
    }
    else if (detector.looking_for_sign) {
        detector.status = IMPOSSIBLE;
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
        else if (!(isalpha(c) || c == '_' || isdigit(c) || c == '.'))  {
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
    else if (detector.looking_for_hex && isxdigit(c)) {
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
    if (detector.just_opened) detector.just_opened = false;

    detector.prev_status = detector.status;

    if (detector.status == TRUE) {
        detector.status = IMPOSSIBLE;
    }
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
        detector.in_literal = true;
        detector.looking_for_open_quote = false;
        detector.just_opened = true;
    }
    else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.in_literal && !detector.looking_for_esc_seq && c == '\\') {
        detector.looking_for_esc_seq = true;
        detector.esc_seq_detector = initial_esc_seq_detector;
        detector.esc_seq_detector = detect_escape_sequence(detector.esc_seq_detector, c);
    }
    else if (detector.in_literal && !detector.looking_for_esc_seq && !in_src_char_set(c)) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote && detector.prev_char != '\\' && detector.looking_for_esc_seq && detector.esc_seq_detector.status == TRUE) {
        detector.status = TRUE;
    }
    else if (c == quote && detector.prev_char != '\\' && detector.looking_for_esc_seq) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == '\'' && detector.prev_char != '\\' && !detector.is_first_char && detector.just_opened) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote && detector.prev_char != '\\') {
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

struct punctuator_detector detect_punctuator(struct punctuator_detector detector, char c) {
    detector.prev_status = detector.status;

    if (detector.char_n == 1) {
        switch (c) {
        case '[':case ']':case '(':case ')':case '{':case '}':case '?':case ';':case ',':case '~':
                detector.status = TRUE;
                detector.n_next_char_options = 0;
                break;
            case '+':case '&':case '|':
                detector.status = TRUE;
                detector.n_next_char_options = 2;
                detector.next_char_options[0] = c;
                detector.next_char_options[1] = '=';
                break;
            case '-':
                detector.status = TRUE;
                detector.n_next_char_options = 3;
                detector.next_char_options[0] = '-';
                detector.next_char_options[1] = '=';
                detector.next_char_options[2] = '>';
                break;
            case '*':case '/':case '!':case '^':
                detector.status = TRUE;
                detector.n_next_char_options = 1;
                detector.next_char_options[0] = '=';
                break;
            case '%':
                detector.status = TRUE;
                detector.n_next_char_options = 3;
                detector.next_char_options[0] = '=';
                detector.next_char_options[1] = ':';
                detector.next_char_options[2] = '>';
                break;
            case '<':
                detector.status = TRUE;
                detector.n_next_char_options = 4;
                detector.next_char_options[0] = '<';
                detector.next_char_options[1] = '=';
                detector.next_char_options[2] = ':';
                detector.next_char_options[3] = '%';
                break;
            case '>':
                detector.status = TRUE;
                detector.n_next_char_options = 2;
                detector.next_char_options[0] = '>';
                detector.next_char_options[1] = '=';
                break;
            case ':':
                detector.status = TRUE;
                detector.n_next_char_options = 1;
                detector.next_char_options[0] = '>';
                break;
            case '.':case '=':case '#':
                detector.status = TRUE;
                detector.n_next_char_options = 1;
                detector.next_char_options[0] = c;
                break;
            default:
                detector.status = IMPOSSIBLE;
        }
    }
    else {
        bool valid = false;
        for (unsigned char i = 0; i < detector.n_next_char_options; i++) {
            if (detector.next_char_options[i] == c) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            detector.status = IMPOSSIBLE;
            return detector;
        }

        if (detector.char_n == 2) {
            switch (c) {
                case '.':
                    detector.status = POSSIBLE;
                    detector.n_next_char_options = 1;
                    detector.next_char_options[0] = '.';
                    break;
                case '<':case '>':
                    if (detector.prev_char == c) {
                        detector.status = TRUE;
                        detector.n_next_char_options = 1;
                        detector.next_char_options[0] = '=';
                    }
                    break;
                case ':':
                    switch (detector.prev_char) {
                        case '%':
                            detector.status = TRUE;
                            detector.n_next_char_options = 1;
                            detector.next_char_options[0] = '%';
                            break;
                        case '<':
                            detector.status = TRUE;
                            detector.n_next_char_options = 0;
                            break;
                    }
                    break;
                default:
                    detector.status = TRUE;
                    detector.n_next_char_options = 0;
            }
        }
        else if (detector.char_n == 3) {
            if (c == '%') {
                detector.status = POSSIBLE;
                detector.n_next_char_options = 1;
                detector.next_char_options[0] = ':';
            }
            else {
                detector.status = TRUE;
                detector.n_next_char_options = 0;
            }
        }
        else if (detector.char_n == 4) {
            detector.status = TRUE;
            detector.n_next_char_options = 0;
        }
    }

    detector.prev_char = c;
    detector.char_n++;

    return detector;
}

struct single_char_detector detect_single_char(struct single_char_detector detector, char c) {
    detector.prev_status = detector.status;

    if (detector.status == IMPOSSIBLE) return detector;
    else if (detector.status == TRUE) detector.status = IMPOSSIBLE;
    else if (isspace(c)) detector.status = IMPOSSIBLE;
    else detector.status = TRUE;
    return detector;
}

struct preprocessing_token_detector detect_preprocessing_token(struct preprocessing_token_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    detector.prev_status = detector.status;

    detector.header_name_detector = detect_header_name(detector.header_name_detector, c);
    detector.identifier_detector = detect_identifier(detector.identifier_detector, c);
    detector.pp_number_detector = detect_pp_number(detector.pp_number_detector, c);
    detector.character_constant_detector = detect_character_constant(detector.character_constant_detector, c);
    detector.string_literal_detector = detect_string_literal(detector.string_literal_detector, c);
    detector.punctuator_detector = detect_punctuator(detector.punctuator_detector, c);
    detector.single_char_detector = detect_single_char(detector.single_char_detector, c);

    if (detector.header_name_detector.status == TRUE || detector.identifier_detector.status == TRUE
    || detector.pp_number_detector.status == TRUE || detector.character_constant_detector.status == TRUE
    || detector.string_literal_detector.status == TRUE || detector.punctuator_detector.status == TRUE
    || detector.single_char_detector.status == TRUE) {
        detector.status = TRUE;
    }
    else if (detector.header_name_detector.status == POSSIBLE || detector.identifier_detector.status == POSSIBLE
             || detector.pp_number_detector.status == POSSIBLE || detector.character_constant_detector.status == POSSIBLE
             || detector.string_literal_detector.status == POSSIBLE || detector.punctuator_detector.status == POSSIBLE
             || detector.single_char_detector.status == POSSIBLE) {
        detector.status = POSSIBLE;
    }
    else {
        detector.status = IMPOSSIBLE;
    }

    if (detector.was_first_char) detector.was_first_char = false;
    if (detector.is_first_char) {
        detector.is_first_char = false;
        detector.was_first_char = true;
    }

    return detector;
}

struct comment_detector detect_comment(struct comment_detector detector, char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    detector.prev_status = detector.status;

    if (detector.next_char_invalid) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char && c != '/') {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char) {}
    else if (detector.is_second_char && c == '/') {
        detector.is_multiline = false;
        detector.status = TRUE;
    }
    else if (detector.is_second_char && c == '*') {
        detector.is_multiline = true;
        detector.status = TRUE;
    }
    else if (detector.is_second_char) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_multiline && detector.prev_char == '*' && c == '/') {
        detector.next_char_invalid = true;
    }
    else if (!detector.is_multiline && c == '\n') {
        detector.status = IMPOSSIBLE;
    }

    if (detector.is_second_char) detector.is_second_char = false;
    if (detector.is_first_char) {
        detector.is_first_char = false;
        detector.is_second_char = true;
    }
    detector.prev_char = c;

    return detector;
}

void add_type(struct preprocessing_token *token, struct preprocessing_token_detector detector) {
    if (detector.header_name_detector.status == TRUE) token->type = HEADER_NAME;
    else if (detector.string_literal_detector.status == TRUE) token->type = STRING_LITERAL;
    else if (detector.identifier_detector.status == TRUE) token->type = IDENTIFIER;
    else if (detector.pp_number_detector.status == TRUE) token->type = PP_NUMBER;
    else if (detector.character_constant_detector.status == TRUE) token->type = CHARACTER_CONSTANT;
    else if (detector.punctuator_detector.status == TRUE) token->type = PUNCTUATOR;
    else if (detector.single_char_detector.status == TRUE) token->type = SINGLE_CHAR;
    else {
        preprocessor_error(0, 0, 0, "momento de bruh");
    }
}

static bool token_is_str(struct preprocessing_token token, const char *str) {
    for (const char *tok_it = token.first, *str_it = str; tok_it != token.last+1 && *str_it; tok_it++, str_it++) {
        if (*tok_it != *str_it) return false;
    }
    return true;
}

pp_token_vec get_pp_tokens(struct lines lines) {
    struct universal_character_name_detector initial_ucnd =
            {.status=POSSIBLE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false};
    struct char_const_str_literal_detector initial_ccsld = {.status=POSSIBLE, .looking_for_open_quote=true, .in_literal=false,
            .prev_esc_seq_status=POSSIBLE, .is_first_char=true, .just_opened=false,
            .esc_seq_detector={.status=POSSIBLE, .looking_for_hex=false, .looking_for_octal=false,
                    .next_char_invalid=false,.is_first_char=true, .is_second_char=false,
                    .n_octals=0, .ucn_detector=initial_ucnd}};
    struct preprocessing_token_detector initial_detector = {
            .status=POSSIBLE,
            .prev_status=POSSIBLE,
            .header_name_detector={.status=POSSIBLE, .is_first_char=true},
            .identifier_detector={.is_first_char=true, .status=POSSIBLE, .ucn_detector=initial_ucnd},
            .pp_number_detector={.status=POSSIBLE, .looking_for_digit=false, .looking_for_ucn=false,
                                 .looking_for_sign=false, .is_first_char=true,.ucn_detector=initial_ucnd},
            .character_constant_detector=initial_ccsld,
            .string_literal_detector=initial_ccsld,
            .punctuator_detector={.status=POSSIBLE, .char_n=1},
            .single_char_detector={.status=POSSIBLE},
            .is_first_char=true,
            .was_first_char=false
    };
    struct preprocessing_token_detector detector = initial_detector;
    struct preprocessing_token_detector prev_detector;
    struct comment_detector initial_comment_detector = {.status = POSSIBLE, .is_first_char=true, .is_second_char=false,
            .next_char_invalid=false};
    struct comment_detector comment_detector = initial_comment_detector;

    pp_token_vec result;
    pp_token_vec_init(&result, 0);

    for (size_t ln_i = 0; ln_i < lines.n_lines; ln_i++) {
        struct preprocessing_token token = {.possible_types = 0};
        for (size_t char_i = 0; char_i < lines.lines[ln_i].n_chars; char_i++) {
            const char *c = &lines.lines[ln_i].chars[char_i];
            comment_detector = detect_comment(comment_detector, *c);
            if (detector.string_literal_detector.in_literal || detector.character_constant_detector.in_literal) {
                comment_detector.status = IMPOSSIBLE;
            }
            else if (comment_detector.status == TRUE) {
                continue;
            }
            else if (comment_detector.status == IMPOSSIBLE && comment_detector.prev_status == TRUE) {
                detector = initial_detector;
            }
            if (comment_detector.status == IMPOSSIBLE) {
                comment_detector = initial_comment_detector;
            }

            if (!(result.n_elements >= 2
                  && token_is_str(result.arr[result.n_elements-1], "include")
                  && token_is_str(result.arr[result.n_elements-2], "#"))) {
                detector.header_name_detector.status = IMPOSSIBLE;
            }

            prev_detector = detector;
            detector = detect_preprocessing_token(detector, *c);

            if ((detector.status == POSSIBLE || detector.status == TRUE) && (detector.was_first_char || detector.prev_status == IMPOSSIBLE)) {
                token.first = c;
            }
            // TODO cleanup oh my god this is awful
            else if (detector.status == IMPOSSIBLE && detector.prev_status == TRUE) {
                token.last = c-1;
                add_type(&token, prev_detector);
                pp_token_vec_append(&result, token);
                token.possible_types = 0;
                char_i--;
                comment_detector = initial_comment_detector;
            }
            if (detector.status == TRUE && char_i == lines.lines[ln_i].n_chars-1) {
                token.last = c;
                add_type(&token, detector);
                pp_token_vec_append(&result, token);
                token.possible_types = 0;
            }
            if (detector.status == IMPOSSIBLE) detector = initial_detector;
        }
    }

    return result;
}
