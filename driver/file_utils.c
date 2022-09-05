
#include "file_utils.h"
#include "diagnostics.h"
#include <errno.h>
#include <limits.h>

char *new_fname_ext(const char *fname, const char *new_ext) {
    char *output = malloc(strlen(fname) + strlen(new_ext) + 1);
    char *dot_p = strrchr(fname, '.');
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

long get_filesize_then_rewind(FILE *file) {
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    if (filesize == -1) {
        switch(errno) {
            case EOVERFLOW:
                // TODO specify name of file
                driver_error("Input file too large. Maximum filesize is %l bytes.", LONG_MAX);
        }
    }
    rewind(file);
    return filesize;
}
