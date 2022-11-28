#ifndef PREPROCESSOR_DETECTOR_H
#define PREPROCESSOR_DETECTOR_H

#include <stdbool.h>

enum detection_status {
    IMPOSSIBLE,
    INCOMPLETE,
    MATCH
};

static inline bool is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

#endif // PREPROCESSOR_DETECTOR_H
