#ifndef TEST_DIAGNOSTICS_H
#define TEST_DIAGNOSTICS_H

#include <stddef.h>

__attribute__((format(printf, 4, 5), noreturn))
void preprocessor_fatal_error(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);
__attribute__((format(printf, 4, 5)))
void preprocessor_error(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);
__attribute__((format(printf, 4, 5)))
void preprocessor_warning(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);


#endif //TEST_DIAGNOSTICS_H
