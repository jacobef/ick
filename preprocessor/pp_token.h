#ifndef ICK_PP_TOKEN_H
#define ICK_PP_TOKEN_H

#include <stdbool.h>
#include <stdio.h>
#include "data_structures/vector.h"
#include "data_structures/trie.h"
#include "sized_str.h"
#include "detector.h"

struct header_name_detector {
    enum detection_status status;
    bool in_quotes;
    bool is_first_char;
};

struct universal_character_name_detector {
    enum detection_status status;
    bool looking_for_digits;
    bool looking_for_uU;
    bool is_first_char;
    unsigned char expected_digits;
    unsigned char n_digits;
};

static const struct universal_character_name_detector initial_ucn_detector = {.status=INCOMPLETE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false};

struct identifier_detector {
    struct universal_character_name_detector ucn_detector;
    enum detection_status status;
    bool looking_for_ucn;
    bool is_first_char;
};

struct pp_number_detector {
    struct universal_character_name_detector ucn_detector;
    enum detection_status status;
    bool accepting_sign;
    bool looking_for_ucn;
    bool looking_for_digit;
    bool is_first_char;
};

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
        .status=INCOMPLETE, .looking_for_hex=false, .looking_for_octal=false, .next_char_invalid=false, .n_octals=0,
        .is_first_char=true, .is_second_char=false,
        .ucn_detector={.status=INCOMPLETE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false}};

struct char_const_str_literal_detector {
    struct escape_sequence_detector esc_seq_detector;
    enum detection_status status;
    enum detection_status prev_esc_seq_status;
    bool looking_for_open_quote;
    bool in_literal;
    bool looking_for_esc_seq;
    bool is_first_char;
    bool just_opened;
    unsigned char prev_char;
};


struct punctuator_detector {
    const struct trie *place_in_trie;
    enum detection_status status;
};

struct comment_detector {
    enum detection_status status;
    enum detection_status prev_status;
    bool is_multiline;
    bool is_first_char;
    bool is_second_char;
    bool next_char_invalid;
    unsigned char prev_char;
};

struct single_char_detector {
    enum detection_status status;
};

enum pp_token_type {
    HEADER_NAME, IDENTIFIER, PP_NUMBER, CHARACTER_CONSTANT, STRING_LITERAL, PUNCTUATOR, SINGLE_CHAR, COMMENT
};

struct preprocessing_token {
    struct str_view name;
    enum pp_token_type type;
    bool after_whitespace;
};

bool in_src_char_set(unsigned char c);

bool token_is_str(struct preprocessing_token token, const char *str);

struct preprocessing_token_detector {
    struct header_name_detector header_name_detector;
    struct identifier_detector identifier_detector;
    struct pp_number_detector pp_number_detector;
    struct char_const_str_literal_detector character_constant_detector;
    struct char_const_str_literal_detector string_literal_detector;
    struct punctuator_detector punctuator_detector;
    struct single_char_detector single_char_detector;
    struct comment_detector comment_detector;
    enum detection_status status;
    enum detection_status prev_status;
    bool is_first_char;
    bool was_first_char;
};
enum exclude_from_detection {EXCLUDE_STRING_LITERAL, EXCLUDE_HEADER_NAME};

typedef struct preprocessing_token pp_token;
DEFINE_VEC_TYPE_AND_FUNCTIONS(pp_token)
pp_token_vec get_pp_tokens(struct str_view input);

bool is_valid_token(struct str_view token, enum exclude_from_detection exclude);
enum pp_token_type get_token_type_from_str(struct str_view token, enum exclude_from_detection exclude);

void print_tokens(pp_token_vec tokens, bool verbose);

static const struct trie punctuators_trie = {
    /*
    punctuators:
    [ ] ( ) { } . ->
    ++ -- & * + - ~ !
    / % << >> < > <= >= == != ^ | && ||
    ? : ; ...
    = *= /= %= += -= <<= >>= &= ^= |=
    , # ##
    <: :> <% %> %: %:%:
    */
    .n_children = 24, .children = (struct trie[]) {
        // (
        (struct trie) {
            .val = '(', .match = true, .n_children = 0
        },
        // )
        (struct trie) {
            .val = ')', .match = true, .n_children = 0
        },
        // [
        (struct trie) {
            .val = '[', .match = true, .n_children = 0
        },
        // ]
        (struct trie) {
            .val = ']', .match = true, .n_children = 0
        },
        // {
        (struct trie) {
            .val = '{', .match = true, .n_children = 0
        },
        // }
        (struct trie) {
            .val = '}', .match = true, .n_children = 0
        },
        // . ...
        (struct trie) {
            .val = '.', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '.', .match = false, .n_children = 1, .children =(struct trie[]) {
                        (struct trie) {
                            .val = '.', .match = true, .n_children = 0
                        }
                    }
                }
            }
        },
        // - -> -= --
        (struct trie) {
            .val = '-', .match = true, .n_children = 3, .children = (struct trie[]) {
                (struct trie) {
                    .val = '>', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '-', .match = true, .n_children = 0
                }
            }
        },
        // + += ++
        (struct trie) {
            .val = '+', .match = true, .n_children = 2, .children = (struct trie[]) {
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '+', .match = true, .n_children = 0
                }
            }
        },
        // & && &=
        (struct trie) {
            .val = '&', .match = true, .n_children = 2, .children = (struct trie[]) {
                (struct trie) {
                    .val = '&', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // * *=
        (struct trie) {
            .val = '*', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // ~
        (struct trie) {
            .val = '~', .match = true, .n_children = 0
        },
        // ! !=
        (struct trie) {
            .val = '!', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // / /=
        (struct trie) {
                .val = '/', .match = true, .n_children = 1, .children = (struct trie[]) {
                        (struct trie) {
                                .val = '=', .match = true, .n_children = 0
                        }
                }
        },
        // % %> %: %:%:
        (struct trie) {
            .val = '%', .match = true, .n_children = 3, .children = (struct trie[]) {
                (struct trie) {
                    .val = '>', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = ':', .match = true, .n_children = 1, .children = (struct trie[]) {
                        (struct trie) {
                            .val = '%', .match = false, .n_children = 1, .children = (struct trie[]) {
                                (struct trie) {
                                    .val = ':', .match = true, .n_children = 0
                                }
                            }
                        }
                    }
                }
            }
        },
        // < << <<= <= <: <%
        (struct trie) {
            .val = '<', .match = true, .n_children = 4, .children = (struct trie[]) {
                (struct trie) {
                    .val = '<', .match = true, .n_children = 1, .children = (struct trie[]) {
                        (struct trie) {
                            .val = '=', .match = true, .n_children = 0
                        }
                    }
                },
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = ':', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '%', .match = true, .n_children = 0
                }
            }
        },
        // > >> >>= >=
        (struct trie) {
            .val = '>', .match = true, .n_children = 2, .children = (struct trie[]) {
                (struct trie) {
                    .val = '>', .match = true, .n_children = 1, .children = (struct trie[]) {
                        (struct trie) {
                            .val = '=', .match = true, .n_children = 0
                        }
                    }
                },
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                },
            }
        },
        // = ==
        (struct trie) {
            .val = '=', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // ^ ^=
        (struct trie) {
            .val = '^', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // | || |=
        (struct trie) {
            .val = '|', .match = true, .n_children = 2, .children = (struct trie[]) {
                (struct trie) {
                    .val = '|', .match = true, .n_children = 0
                },
                (struct trie) {
                    .val = '=', .match = true, .n_children = 0
                }
            }
        },
        // ?
        (struct trie) {
            .val = '?', .match = true, .n_children = 0
        },
        // : :>
        (struct trie) {
            .val = ':', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '>', .match = true, .n_children = 0
                }
            }
        },
        // ,
        (struct trie) {
            .val = ',', .match = true, .n_children = 0
        },
        // # ##
        (struct trie) {
            .val = '#', .match = true, .n_children = 1, .children = (struct trie[]) {
                (struct trie) {
                    .val = '#', .match = true, .n_children = 0
                }
            }
        }
    }
};

#endif //ICK_PP_TOKEN_H
