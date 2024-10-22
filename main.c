#include <stdio.h>
#include <string.h>
#include "data_structures/vector.h"
#include "driver/file_utils.h"
#include "driver/diagnostics.h"
#include "preprocessor/trigraphs.h"
#include "preprocessor/escaped_newlines.h"
#include "preprocessor/pp_token.h"
#include "preprocessor/parser.h"

char *ick_progname;

int main(int argc, char *argv[]) {
#ifdef DEBUG
    atexit(check_reminders);
#endif
    if (argc == 0 || (argv[0][0] == '\0')) ick_progname = "ick";
    else { // set ick_progname to the basename of argv[0]
        const size_t argv0_len = strlen(argv[0]);
        ick_progname = MALLOC(argv0_len + 1);
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
    FREE(output_fname);

    const size_t input_len = get_filesize(input_file);
    unsigned char *input_chars = MALLOC(input_len+1);
    fread(input_chars, sizeof(unsigned char), input_len, input_file);
    fclose(input_file);
    input_chars[input_len] = '\n'; // too much of a pain without this

    const struct trigraph_replacement_info trigraph_replacement = replace_trigraphs(
            (sstr){ .data = input_chars, .len = input_len + 1 }
    );
    const struct escaped_newlines_replacement_info logical_lines = rm_escaped_newlines(trigraph_replacement.result);
    fwrite(logical_lines.result.data, sizeof(char), logical_lines.result.len, output_file);

    fclose(output_file);

    const pp_token_harr tokens = get_pp_tokens(logical_lines.result);
//    print_tokens(tokens);
    
    test_parser(tokens);

    FREE(input_chars);
    FREE(trigraph_replacement.result.data);
    FREE(logical_lines.result.data);
    FREE(ick_progname);
}
