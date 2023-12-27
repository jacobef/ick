#include "file_utils.h"
#include "diagnostics.h"
#include "debug/malloc.h"
#include <errno.h>
#include <limits.h>

char *new_fname_ext(const char *fname, const char *new_ext) {
    char *output = MALLOC(strlen(fname) + strlen(new_ext) + 1);
    const char *dot_p = strrchr(fname, '.');
    if (dot_p != NULL && dot_p != fname) {
        strncpy(output, fname, dot_p - fname);
        output[dot_p-fname] = '\0';
    }
    else {
        strcpy(output, fname);
    }
    strcat(output, new_ext);
    return output;
}

unsigned long get_filesize(FILE *file) {
    long file_pos = ftell(file);
    if (file_pos == -1) {
        driver_error("Error in getting the initial file position. Errno: %d (%s).", errno, strerror(errno));
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    if (filesize == -1) {
        driver_error("Input file is likely too large (ftell failed to seek to the end). Errno: %d (%s).", errno, strerror(errno));
    } else if ((unsigned long)filesize >= SIZE_MAX) {
        driver_error("Input file is too large (%l bytes); should be no larger than SIZE_MAX (%zu)", filesize, SIZE_MAX);
    }
    fseek(file, file_pos, SEEK_SET);
    return (unsigned long)filesize;
}
