#ifndef ICK_FILE_UTILS_H
#define ICK_FILE_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ASM_EXT ".s"
#define PREPROCESSED_EXT ".i"
#define C_EXT ".c"

/*!
 * Returns a malloc'd string, which is fname with its file extension changed to new_ext.
 * Specifically:\n
 * 1) The file extension is removed, if it exists.
 * The extension is considered to start at (and include) the last dot that isn't the first character.\n
 * 2) new_ext is appended to the string.
 * @param fname The string with the old extension.
 * @param new_ext The new extension. Should probably start with a dot; see examples.
 * @example
 * \code{.c}
 * char some_c_file[5] = "hi.c"
 * char *some_asm_file = change_fname_ext(some_c_file, ".s");
 * printf("%s", some_asm_file); // prints "hi.s"
 * \endcode
 * @example
 * The following examples are shorthand:\n
 * ("hi.ab", ".s") -> "hi.s"\n
 *  ("hi", ".e") -> "hi.e"\n
 * ("hi.ab", "") -> "hi"\n
 * ("hi.ab.cd", ".ef") -> "hi.ab.ef"\n
 * (".hi", ".bye") -> ".hi.bye"\n
 * ("hi.", "") -> "hi"
 */
char *new_fname_ext(const char *fname, const char *new_ext);

/*
 * Returns the number of bytes in the file, not including EOF.
 */
unsigned long get_filesize(FILE *file);

#endif //ICK_FILE_UTILS_H
