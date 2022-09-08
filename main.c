#include <stdio.h>
#include <string.h>
#include "data_structures/vector.h"
#include "driver/file_utils.h"
#include "driver/diagnostics.h"
#include "preprocessor/lines.h"
#include "preprocessor/trigraphs.h"
#include "preprocessor/escaped_newlines.h"
#include "preprocessor/diagnostics.h"
#include "preprocessor/pp_token.h"

char *ick_progname;

int main(int argc, char *argv[]) {
    if (argc == 0 || (argv[0][0] == '\0')) ick_progname = "ick";
    else {
        size_t argv0_len = strlen(argv[0]);
        ick_progname = malloc(argv0_len + 1);
        const char *last_path_sep;
        for (last_path_sep = argv[0]+argv0_len-1;
        last_path_sep >= argv[0] && *last_path_sep != '/' && *last_path_sep != '\\';
        last_path_sep--);
        strcpy(ick_progname, last_path_sep + 1); // probably unsafe
    }

    if (argc <= 1) {
        driver_error("No target file(s) specified.");
    }

    const char *input_fname = argv[1];

    char *output_fname = new_fname_ext(input_fname, PREPROCESSED_EXT);

    if (strcmp(input_fname, output_fname) == 0) {
        // TODO when -o support is added, make message depend on whether -o is passed
        driver_error("The output filename, \"%s\", is the same as the input filename.", output_fname);
    }

    FILE *input_file = fopen(input_fname, "r");
    if (input_file == NULL) {
        driver_error("Input file \"%s\" does not exist.", input_fname);
    }

    FILE *output_file = fopen(output_fname, "w");
    free(output_fname);

    long input_len = get_filesize_then_rewind(input_file);
    char *input_chars = malloc(input_len);
    fread(input_chars, sizeof(char), input_len, input_file);
    fclose(input_file);

    struct lines source_lines = get_lines(input_chars, input_len);
    struct lines trigraphs_replaced = replace_trigraphs(source_lines);
    struct lines logical_lines = rm_escaped_newlines(trigraphs_replaced);
    fwrite(logical_lines.chars, sizeof(char), logical_lines.n_chars, output_file);

    fclose(output_file);

    struct universal_character_name_detector initial_ucnd = {.status=POSSIBLE, .is_first_char=true, .n_digits=0, .looking_for_uU=false, .looking_for_digits=false};
    struct identifier_detector idd = {.is_first_char=true, .status=POSSIBLE,
            .ucn_detector=initial_ucnd};
    struct pp_number_detector pnd = {.status=POSSIBLE, .looking_for_digit=false, .looking_for_ucn=false, .looking_for_sign=false, .is_first_char=true,
            .ucn_detector=initial_ucnd};
    struct escape_sequence_detector esd = {.status=POSSIBLE, .looking_for_hex=false, .looking_for_octal=false, .next_char_invalid=false,
            .is_first_char=true, .is_second_char=false, .n_octals=0, .ucn_detector=initial_ucnd};
    struct char_const_str_literal_detector ccd = {.status=POSSIBLE, .esc_seq_detector=esd, .looking_for_open_quote=true, .looking_for_char_seq=false,
            .prev_esc_seq_status=POSSIBLE, .is_first_char=true};
    ccd = detect_character_constant_test(ccd, "L\'\\U123456789\'");

    free(source_lines.chars);
    if (source_lines.chars != trigraphs_replaced.chars) free(trigraphs_replaced.chars);
    free(ick_progname);
}
