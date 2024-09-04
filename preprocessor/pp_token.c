// ReSharper disable CppDFAUnreachableCode
#include <ctype.h>
#include "detector.h"
#include "pp_token.h"
#include "preprocessor/diagnostics.h"

bool in_src_char_set(const unsigned char c) {
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

static bool is_octal_digit(const unsigned char c) {
    return c >= '0' && c <= '7';
}

static struct header_name_detector detect_header_name(struct header_name_detector detector, const unsigned char c) {
    if (detector.status == MATCH) detector.status = IMPOSSIBLE;
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
        detector.status = MATCH;
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

static struct universal_character_name_detector detect_universal_character_name(struct universal_character_name_detector detector, const unsigned char c) {
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
            detector.status = MATCH;
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

static struct identifier_detector detect_identifier(struct identifier_detector detector, const unsigned char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.is_first_char && isdigit(c)) {
        detector.status = IMPOSSIBLE;
    } else if (c == '\\' && !detector.looking_for_ucn) {
        detector.looking_for_ucn = true;
        detector.ucn_detector = initial_ucn_detector;
        detector.status = INCOMPLETE;
    } else if (c == '\\') {
        detector.status = IMPOSSIBLE;
    } else if (!detector.looking_for_ucn && (isalpha(c) || c == '_' || isdigit(c))) {
        detector.status = MATCH;
    } else if (!detector.looking_for_ucn) {
        detector.status = IMPOSSIBLE;
    }

    if (detector.looking_for_ucn) {
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
        if (detector.ucn_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
        } else if (detector.ucn_detector.status == MATCH) {
            detector.looking_for_ucn = false;
            detector.status = MATCH;
        }
    }

    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

static struct pp_number_detector detect_pp_number(struct pp_number_detector detector, const unsigned char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.is_first_char && isdigit(c)) {
        detector.status = MATCH;
    } else if (detector.is_first_char && c == '.') {
        detector.looking_for_digit = true;
        detector.status = INCOMPLETE;
    } else if (detector.is_first_char) {
        detector.status = IMPOSSIBLE;
    } else if (detector.looking_for_digit && isdigit(c)) {
        detector.looking_for_digit = false;
        detector.status = MATCH;
    } else if (detector.looking_for_digit) {
        detector.status = IMPOSSIBLE;
    } else if (detector.accepting_sign && (c == '+' || c == '-')) {
        detector.status = MATCH;
    } else if (c == '+' || c == '-') {
        detector.status = IMPOSSIBLE;
    } else if (detector.status == MATCH) {
        if (c == 'e' || c == 'E' || c == 'p' || c == 'P') {
            detector.accepting_sign = true;
        } else if (c == '\\') {
            detector.looking_for_ucn = true;
            detector.status = INCOMPLETE;
        } else if (!(isalpha(c) || c == '_' || isdigit(c) || c == '.'))  {
            detector.status = IMPOSSIBLE;
        }
    }

    if (detector.looking_for_ucn) {
        detector.ucn_detector = detect_universal_character_name(detector.ucn_detector, c);
        if (detector.ucn_detector.status == IMPOSSIBLE) {
            detector.status = IMPOSSIBLE;
        } else if (detector.ucn_detector.status == MATCH) {
            detector.looking_for_ucn = false;
            detector.status = MATCH;
        }
    }
    if (detector.accepting_sign) detector.accepting_sign = false;
    if (detector.is_first_char) detector.is_first_char = false;

    return detector;
}

static struct escape_sequence_detector detect_escape_sequence(struct escape_sequence_detector detector, const unsigned char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.next_char_invalid) {
        detector.status = IMPOSSIBLE;
    }
    else if (detector.is_first_char && c == '\\') {
    }
    else if (detector.is_second_char && (c == '\'' || c == '"' || c == '?' || c == '\\' ||
                c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v')) {
        detector.status = MATCH;
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
        else if (detector.ucn_detector.status == MATCH) {
            detector.status = MATCH;
            detector.looking_for_ucn = false;
        }
    }
    else if (detector.is_second_char && is_octal_digit(c)) {
        detector.n_octals++;
        detector.looking_for_octal = true;
        detector.status = MATCH;
    }
    else if (detector.looking_for_octal && is_octal_digit(c)) {
        detector.n_octals++;
        if (detector.n_octals == 3) detector.next_char_invalid = true;
    }
    else if (detector.looking_for_hex && isxdigit(c)) {
        detector.status = MATCH;
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

static struct char_const_str_literal_detector detect_char_const_str_literal(struct char_const_str_literal_detector detector, const char quote, const unsigned char c) {
    if (detector.just_opened) detector.just_opened = false;

    if (detector.status == MATCH) {
        detector.status = IMPOSSIBLE;
    }
    if (detector.status == IMPOSSIBLE) return detector;

    if (detector.looking_for_esc_seq) {
        detector.esc_seq_detector = detect_escape_sequence(detector.esc_seq_detector, c);
        if (detector.esc_seq_detector.status == IMPOSSIBLE && detector.prev_esc_seq_status == MATCH) {
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
    else if (detector.looking_for_open_quote) {
        detector.status = IMPOSSIBLE;
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
    else if (c == quote && detector.prev_char != '\\' && detector.looking_for_esc_seq && detector.esc_seq_detector.status == MATCH) {
        detector.status = MATCH;
    }
    else if (c == quote && detector.prev_char != '\\' && detector.looking_for_esc_seq) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == '\'' && detector.prev_char != '\\' && !detector.is_first_char && detector.just_opened) {
        detector.status = IMPOSSIBLE;
    }
    else if (c == quote && detector.prev_char != '\\') {
        detector.status = MATCH;
    }

    if (detector.is_first_char) detector.is_first_char = false;
    detector.prev_char = c;

    return detector;
}

static struct char_const_str_literal_detector detect_character_constant(struct char_const_str_literal_detector detector, const unsigned char c) {
    return detect_char_const_str_literal(detector, '\'', c);
}

static struct char_const_str_literal_detector detect_string_literal(struct char_const_str_literal_detector detector, const unsigned char c) {
    return detect_char_const_str_literal(detector, '"', c);
}

static struct punctuator_detector detect_punctuator(struct punctuator_detector detector, const unsigned char c) {

    if (detector.status == IMPOSSIBLE) return detector;

    const struct trie *const next_place_in_trie = trie_get_child(detector.place_in_trie, c);
    if (next_place_in_trie == NULL) {
        detector.status = IMPOSSIBLE;
    }
    else {
        if (next_place_in_trie->match) detector.status = MATCH;
        else detector.status = INCOMPLETE;
        detector.place_in_trie = next_place_in_trie;
    }
    return detector;
}

static struct single_char_detector detect_single_char(struct single_char_detector detector, const unsigned char c) {
    if (detector.status == IMPOSSIBLE) return detector;
    else if (detector.status == MATCH) detector.status = IMPOSSIBLE;
    // newlines aren't actually tokens but they're significant in phase 4, so it's easier to treat them as tokens
    else if (isspace(c) && c != '\n') detector.status = IMPOSSIBLE;
    else detector.status = MATCH;
    return detector;
}

static struct comment_detector detect_comment(struct comment_detector detector, const unsigned char c) {
    if (detector.status == IMPOSSIBLE) return detector;

    detector.prev_status = detector.status;

    if (detector.next_char_invalid) {
        detector.status = IMPOSSIBLE;
    } else if (detector.is_first_char && c != '/') {
        detector.status = IMPOSSIBLE;
    } else if (detector.is_first_char) {}
    else if (detector.is_second_char && c == '/') {
        detector.is_multiline = false;
        detector.status = MATCH;
    } else if (detector.is_second_char && c == '*') {
        detector.is_multiline = true;
        detector.status = MATCH;
    } else if (detector.is_second_char) {
        detector.status = IMPOSSIBLE;
    } else if (detector.is_multiline && detector.prev_char == '*' && c == '/') {
        detector.next_char_invalid = true;
    } else if (!detector.is_multiline && c == '\n') {
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

static struct preprocessing_token_detector detect_preprocessing_token(struct preprocessing_token_detector detector, const unsigned char c, const enum exclude_from_detection exclude) {
    if (detector.status == IMPOSSIBLE) return detector;

    detector.prev_status = detector.status;

    if (exclude == EXCLUDE_HEADER_NAME) {
        detector.string_literal_detector = detect_string_literal(detector.string_literal_detector, c);
        detector.header_name_detector.status = IMPOSSIBLE;
    } else if (exclude == EXCLUDE_STRING_LITERAL) {
        detector.header_name_detector = detect_header_name(detector.header_name_detector, c);
        detector.string_literal_detector.status = IMPOSSIBLE;
    } else {
        preprocessor_fatal_error(0, 0, 0, "invalid value of exclude parameter in detect_preprocessing_token");
    }
    detector.identifier_detector = detect_identifier(detector.identifier_detector, c);
    detector.pp_number_detector = detect_pp_number(detector.pp_number_detector, c);
    detector.character_constant_detector = detect_character_constant(detector.character_constant_detector, c);
    detector.punctuator_detector = detect_punctuator(detector.punctuator_detector, c);
    detector.single_char_detector = detect_single_char(detector.single_char_detector, c);
    detector.comment_detector = detect_comment(detector.comment_detector, c);

    if (detector.header_name_detector.status == MATCH || detector.identifier_detector.status == MATCH
        || detector.pp_number_detector.status == MATCH || detector.character_constant_detector.status == MATCH
        || detector.string_literal_detector.status == MATCH || detector.punctuator_detector.status == MATCH
        || detector.single_char_detector.status == MATCH || detector.comment_detector.status == MATCH) {
        detector.status = MATCH;
    } else if (detector.header_name_detector.status == INCOMPLETE || detector.identifier_detector.status == INCOMPLETE
             || detector.pp_number_detector.status == INCOMPLETE || detector.character_constant_detector.status == INCOMPLETE
             || detector.string_literal_detector.status == INCOMPLETE || detector.punctuator_detector.status == INCOMPLETE
             || detector.single_char_detector.status == INCOMPLETE || detector.comment_detector.status == INCOMPLETE) {
        detector.status = INCOMPLETE;
    } else {
        detector.status = IMPOSSIBLE;
    }

    return detector;
}

static enum pp_token_type get_token_type(const struct preprocessing_token_detector detector) {
    if (detector.string_literal_detector.status == MATCH) return STRING_LITERAL;
    else if (detector.header_name_detector.status == MATCH) return HEADER_NAME;
    else if (detector.identifier_detector.status == MATCH) return IDENTIFIER;
    else if (detector.pp_number_detector.status == MATCH) return PP_NUMBER;
    else if (detector.character_constant_detector.status == MATCH) return CHARACTER_CONSTANT;
    else if (detector.punctuator_detector.status == MATCH) return PUNCTUATOR;
    else if (detector.comment_detector.status == MATCH) return COMMENT;
    else if (detector.single_char_detector.status == MATCH) return SINGLE_CHAR;
    else {
        preprocessor_fatal_error(0, 0, 0, "token doesn't seem to have a type");
    }
}

bool token_is_str(const struct preprocessing_token token, const char *const str) {
    return str_view_cstr_eq(token.name, str);
}

static bool in_include_directive(const pp_token_vec tokens) {
    const bool at_beginning_of_file = tokens.n_elements == 2;
    const bool after_hashtag_include = tokens.n_elements >= 2
                                        && token_is_str(tokens.arr[tokens.n_elements - 2], "#")
                                        && token_is_str(tokens.arr[tokens.n_elements - 1], "include");
    const bool hashtag_after_newline = tokens.n_elements >= 3
                                        && token_is_str(tokens.arr[tokens.n_elements - 3], "\n");
    return after_hashtag_include && (at_beginning_of_file || hashtag_after_newline);
}

static struct preprocessing_token_detector get_initial_detector(void) {
    const struct char_const_str_literal_detector initial_ccsld = {.status=INCOMPLETE, .looking_for_open_quote=true, .in_literal=false,
            .prev_esc_seq_status=INCOMPLETE, .is_first_char=true, .just_opened=false,
            .esc_seq_detector={.status=INCOMPLETE, .looking_for_hex=false, .looking_for_octal=false,
                    .next_char_invalid=false,.is_first_char=true, .is_second_char=false,
                    .n_octals=0, .ucn_detector=initial_ucn_detector}};
    const struct preprocessing_token_detector initial_detector = {
            .status=INCOMPLETE,
            .prev_status=INCOMPLETE,
            .header_name_detector={.status=INCOMPLETE, .is_first_char=true},
            .identifier_detector={.is_first_char=true, .status=INCOMPLETE, .ucn_detector=initial_ucn_detector},
            .pp_number_detector={.status=INCOMPLETE, .looking_for_digit=false, .looking_for_ucn=false,
                    .accepting_sign=false, .is_first_char=true,.ucn_detector=initial_ucn_detector},
            .character_constant_detector=initial_ccsld,
            .string_literal_detector=initial_ccsld,
            .punctuator_detector={.status=INCOMPLETE, .place_in_trie=&punctuators_trie},
            .single_char_detector={.status=INCOMPLETE},
            .comment_detector={.status=INCOMPLETE, .prev_status=INCOMPLETE, .is_multiline=false,
                    .is_first_char=true, .is_second_char=false, .next_char_invalid=false}
    };
    return initial_detector;
}

bool is_valid_token(const struct str_view token, const enum exclude_from_detection exclude) {
    struct preprocessing_token_detector detector = get_initial_detector();
    for (size_t i = 0; i < token.n; i++) {
        detector = detect_preprocessing_token(detector, token.chars[i], exclude);
    }
    return detector.status == MATCH;
}

enum pp_token_type get_token_type_from_str(const struct str_view token, const enum exclude_from_detection exclude) {
    struct preprocessing_token_detector detector = get_initial_detector();
    for (size_t i = 0; i < token.n; i++) {
        detector = detect_preprocessing_token(detector, token.chars[i], exclude);
    }
    return get_token_type(detector);
}


pp_token_vec get_pp_tokens(const struct str_view input) {
    // TODO:
    // Error on invalid tokens.
    // Currently, it skips over invalid tokens instead of erroring.
    // This is fine in C because it only ends up skipping over whitespace (since any single character is a valid token).
    // But if C was different, then it would skip over invalid tokens, when it should error.

    pp_token_vec tokens = pp_token_vec_new(input.n / 3);  // guess 3 chars per token
    bool match_exists = false;
    struct preprocessing_token_detector token_detector = get_initial_detector();

    size_t token_start = 0;
    struct preprocessing_token token_at_most_recent_match;
    for (size_t i = 0; i < input.n; i++) {
        // The token can't be a string literal if it's after #include, and it can't be a header name if it's not
        token_detector = detect_preprocessing_token(token_detector, input.chars[i],
                                                    in_include_directive(tokens) ? EXCLUDE_STRING_LITERAL : EXCLUDE_HEADER_NAME);
        // If the token is valid, then indicate that and set token_at_most_recent_match to the token
        if (token_detector.status == MATCH) {
            match_exists = true;
            const bool after_actual_whitespace = token_start != 0 && isspace(input.chars[token_start-1]);
            const bool after_comment = tokens.n_elements > 0 && tokens.arr[tokens.n_elements - 1].type == COMMENT;
            token_at_most_recent_match = (struct preprocessing_token) {
                .after_whitespace = after_actual_whitespace || after_comment,
                .name = { .chars = &input.chars[token_start], .n = i-token_start + 1 },
                .type = get_token_type(token_detector)
            };
        } else if (token_detector.status == IMPOSSIBLE) {
            if (match_exists) {
                // If the token was valid before, then add that valid token
                pp_token_vec_append(&tokens, token_at_most_recent_match);
                match_exists = false;
                // Set the iterator to right after that token (-1 because char_p gets incremented at the end of the loop)
                i = token_start + token_at_most_recent_match.name.n - 1;
                // Start a new token at the iterator
                token_start = i+1;
            } else {
                // Try starting from the next character
                token_start++;
            }
            token_detector = get_initial_detector();
        }
    }
    if (match_exists) {
        // If there's a lingering valid token (happens when the last character is part of a valid token), add it
        pp_token_vec_append(&tokens, token_at_most_recent_match);
    }

    // Remove the comments
    pp_token_vec tokens_without_comments = pp_token_vec_new(tokens.n_elements);
    for (size_t i = 0; i < tokens.n_elements; i++) {
        if (tokens.arr[i].type != COMMENT) {
            pp_token_vec_append(&tokens_without_comments, tokens.arr[i]);
        }
    }
    pp_token_vec_free_internals(&tokens);

    print_tokens(tokens_without_comments, true);

    return tokens_without_comments;
}

void print_tokens(const pp_token_vec tokens, const bool verbose) {
    if (verbose) {
        for (size_t i = 0; i < tokens.n_elements; i++) {
            const struct preprocessing_token token = tokens.arr[i];
            for (size_t j = 0; j < token.name.n; j++) {
                if (token.name.chars[j] == '\n') {
                    printf("[newline]");
                } else {
                    printf("%c", token.name.chars[j]);
                }
            }
            printf(" (");
            if (token.type == HEADER_NAME) printf("header name");
            else if (token.type == IDENTIFIER) printf("identifier");
            else if (token.type == PP_NUMBER) printf("preprocessing number");
            else if (token.type == CHARACTER_CONSTANT) printf("character constant");
            else if (token.type == STRING_LITERAL) printf("string literal");
            else if (token.type == PUNCTUATOR) printf("punctuator");
            else if (token.type == SINGLE_CHAR) printf("single character");
            printf(")");
            if (token.after_whitespace) printf(" (after whitespace)");
            printf("\n");
        }
    }

    // print normally
    for (size_t i = 0; i < tokens.n_elements; i++) {
        const struct preprocessing_token token = tokens.arr[i];
        if (token.after_whitespace) printf(" ");
        for (size_t j = 0; j < token.name.n; j++) {
            printf("%c", token.name.chars[j]);
        }
    }
    printf("\n");
}
