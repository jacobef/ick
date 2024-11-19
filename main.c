#include <stdio.h>
#include <string.h>
#include "data_structures/vector.h"
#include "driver/file_utils.h"
#include "driver/diagnostics.h"
#include "preprocessor/trigraphs.h"
#include "preprocessor/escaped_newlines.h"
#include "preprocessor/pp_token.h"
#include "preprocessor/parser.h"
#include "preprocessor/preprocessor.h"

char *ick_progname;

int main(int argc, char *argv[]) {
#ifdef DEBUG
    atexit(check_reminders);
#endif
    if (argc == 0 || (argv[0][0] == '\0')) ick_progname = "ick";
    else { // set ick_progname to the basename of argv[0]
        // TODO fix for unix filenames that contain a backslash
        ssize_t i;
        for (i = (ssize_t)strlen(argv[0])-1;
        i >= 0 && argv[0][i] != '/' && argv[0][i] != '\\';
        i--);
        ick_progname = &argv[0][i+1];
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

    const pp_token_harr preprocessed_tokens = preprocess_file(input_file);
    print_tokens(output_file, preprocessed_tokens, false, false);
}
