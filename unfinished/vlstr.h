
#ifndef ICK_DATA_STRUCTURES_VECTOR_VLSTR_H
#define ICK_DATA_STRUCTURES_VECTOR_VLSTR_H

#include <stdbool.h>
#include "data_structures/vector.h"

// Variable length string
typedef char_vec vlstr;

void vlstr_init(vlstr *vlstr_p, size_t initial_len);
void vlstr_init_with_cstr(vlstr *vlstr_p, const char *cstr);

void vlstr_append(vlstr *dest, const vlstr *to_append);
void vlstr_append_cstr(vlstr *dest, const char *to_append);
void vlstr_append_sized_cstr(vlstr *dest, const char *to_append, size_t to_append_len);
void vlstr_append_char(vlstr *dest, char chr);

//bool vlstr_contains_char(vlstr *vlstr_p, char chr);

const char *vlstr_as_cstr(const vlstr *vlstr_p);
size_t vlstr_len(const vlstr *vlstr_p);

char *vlstr_move_into_cstr(vlstr *vlstr_p);
void vlstr_free(vlstr *vlstr_p);

#endif //ICK_DATA_STRUCTURES_VECTOR_VLSTR_H
