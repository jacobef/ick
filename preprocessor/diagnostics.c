#include "diagnostics.h"
#include <stdarg.h>
#include <stdlib.h>

#define VFPRINTF_VAARGS_FOLLOW(_stream, _fmt) \
    va_list args;                             \
    va_start(args, _fmt);                     \
    vfprintf(_stream, _fmt, args);            \
    va_end(args)

static void preprocessor_message_prefix(FILE *stream, const size_t line, const size_t first_char, const size_t last_char) {
    if (first_char == last_char)
        fprintf(stream, "%s (line %zu, char %zu): ", "insert filename here", line, first_char);
    else
        fprintf(stream, "%s (line %zu, chars %zu-%zu): ", "insert filename here", line, first_char, last_char);
}

void preprocessor_fatal_error(const size_t line, const size_t first_char, const size_t last_char, const char *msg_fmt, ...) {
    preprocessor_message_prefix(stderr, line, first_char, last_char);
    fprintf(stderr, "fatal error: ");
    VFPRINTF_VAARGS_FOLLOW(stderr, msg_fmt);
    fprintf(stderr, "\n");
    exit(1);
}

void preprocessor_error(const size_t line, const size_t first_char, const size_t last_char, const char *msg_fmt, ...) {
    preprocessor_message_prefix(stderr, line, first_char, last_char);
    fprintf(stderr, "error: ");
    VFPRINTF_VAARGS_FOLLOW(stderr, msg_fmt);
    fprintf(stderr, "\n");
}

void preprocessor_warning(const size_t line, const size_t first_char, const size_t last_char, const char *msg_fmt, ...) {
    preprocessor_message_prefix(stderr, line, first_char, last_char);
    fprintf(stderr, "warning: ");
    VFPRINTF_VAARGS_FOLLOW(stderr, msg_fmt);
    fprintf(stderr, "\n");
}
