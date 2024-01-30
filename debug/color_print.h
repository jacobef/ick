//
// Created by Jacob Friedman on 1/30/24.
//

#ifndef ICK_COLOR_PRINT_H
#define ICK_COLOR_PRINT_H

enum text_color { TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_YELLOW, TEXT_COLOR_BLUE,
        TEXT_COLOR_MAGENTA, TEXT_COLOR_CYAN, TEXT_COLOR_WHITE };

void set_color(enum text_color color);
void clear_color(void);

__attribute__((format(printf, 2, 3)))
void print_with_color(enum text_color color, const char *text, ...);

#endif //ICK_COLOR_PRINT_H
