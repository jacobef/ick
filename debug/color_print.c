//
// Created by Jacob Friedman on 1/30/24.
//

#include "color_print.h"
#include <stdio.h>
#include <stdarg.h>

void set_color(const enum text_color color) {
    const char *color_code;
    switch (color) {
        case TEXT_COLOR_RED:
            color_code = "\033[0;31m";
            break;
        case TEXT_COLOR_LIGHT_RED:
            color_code = "\033[1;31m";
            break;
        case TEXT_COLOR_GREEN:
            color_code = "\033[0;32m";
            break;
        case TEXT_COLOR_LIGHT_GREEN:
            color_code = "\033[1;32m";
            break;
        case TEXT_COLOR_BROWN_ORANGE:
            color_code = "\033[0;33m";
            break;
        case TEXT_COLOR_YELLOW:
            color_code = "\033[1;33m";
            break;
        case TEXT_COLOR_BLUE:
            color_code = "\033[0;34m";
            break;
        case TEXT_COLOR_LIGHT_BLUE:
            color_code = "\033[1;34m";
            break;
        case TEXT_COLOR_MAGENTA:
            color_code = "\033[0;35m";
            break;
        case TEXT_COLOR_LIGHT_PURPLE:
            color_code = "\033[1;35m";
            break;
        case TEXT_COLOR_CYAN:
            color_code = "\033[0;36m";
            break;
        case TEXT_COLOR_LIGHT_CYAN:
            color_code = "\033[1;36m";
            break;
        case TEXT_COLOR_WHITE:
            color_code = "\033[0;37m";
            break;
        case TEXT_COLOR_BOLD_WHITE:
            color_code = "\033[1;37m";
            break;
    }
    printf("%s", color_code);
}

void clear_color(void) {
    printf("\033[0m");
}

__attribute__((format(printf, 2, 3)))
void print_with_color(const enum text_color color, const char *const text, ...) {
    va_list args;
    va_start(args, text);
    set_color(color);
    vprintf(text, args);
    clear_color();
    va_end(args);
}
