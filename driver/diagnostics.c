#include "diagnostics.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


#define VFPRINTF_VAARGS_FOLLOW(_stream, _fmt) \
    va_list args;                             \
    va_start(args, _fmt);                     \
    vfprintf(_stream, _fmt, args);            \
    va_end(args)


static void driver_message_prefix(FILE *stream) {
    fprintf(stream, "%s: ", ick_progname);
}

__attribute__((format(printf, 1, 2), noreturn))
void driver_error(const char *msg_fmt, ...) {
    driver_message_prefix(stderr);
    fprintf(stderr, "error: ");
    VFPRINTF_VAARGS_FOLLOW(stderr, msg_fmt);
    fprintf(stderr, "\n");
    exit(1);
}

__attribute__((format(printf, 1, 2)))
void driver_warning(const char *msg_fmt, ...) {
    driver_message_prefix(stderr);
    fprintf(stderr, "warning: ");
    VFPRINTF_VAARGS_FOLLOW(stderr, msg_fmt);
    fprintf(stderr, "\n");
}
