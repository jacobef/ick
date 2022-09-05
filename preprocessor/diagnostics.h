//
// Created by Jacob Friedman on 8/30/22.
//

#ifndef TEST_DIAGNOSTICS_H
#define TEST_DIAGNOSTICS_H

#include <stdio.h>

void preprocessor_fatal_error(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);
void preprocessor_error(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);
void preprocessor_warning(size_t line, size_t first_char, size_t last_char, const char *msg_fmt, ...);


#endif //TEST_DIAGNOSTICS_H
