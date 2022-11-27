#include <stdio.h>
#include <string.h>
#include "data_structures/vector.h"
#include "driver/file_utils.h"
#include "driver/diagnostics.h"
#include "preprocessor/lines.h"
#include "preprocessor/trigraphs.h"
#include "preprocessor/escaped_newlines.h"
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
    char *input_chars = malloc(input_len+1);
    fread(input_chars, sizeof(char), input_len, input_file);
    fclose(input_file);
    input_chars[input_len] = '\n'; // too much of a pain without this

    struct chars trigraphs_replaced = replace_trigraphs(
            (struct chars){ .chars = input_chars, .n_chars = input_len + 1 }
            );
    struct chars logical_lines = rm_escaped_newlines(trigraphs_replaced);
    fwrite(logical_lines.chars, sizeof(char), logical_lines.n_chars, output_file);

    fclose(output_file);

    pp_token_vec pp_tokens = get_pp_tokens(logical_lines);
    for (size_t i = 0; i < pp_tokens.n_elements; i++) {
        struct preprocessing_token token = pp_tokens.arr[i];
        const char *it = token.first;
        while (it != token.last+1) {
            switch(*it) {
                case ' ':
                    if (token.last-token.first == 0) printf("[space]");
                    else printf(" ");
                    break;
                case '\t':
                    printf("[tab]");
                    break;
                case '\n':
                    printf("[newline]");
                    break;
                default:
                    printf("%c", *it);
            }
            it++;
        }
        printf (" (");
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

    pp_token_vec_free_internals(&pp_tokens);
    free(input_chars);
    free(trigraphs_replaced.chars);
    free(logical_lines.chars);
    free(ick_progname);
}
