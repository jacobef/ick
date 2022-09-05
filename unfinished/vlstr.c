
#include "vlstr.h"

void vlstr_init(vlstr *vlstr_p, size_t initial_len) {
    char_vec_init(vlstr_p, initial_len+1);
    char_vec_append(vlstr_p, '\0');
}

void vlstr_init_with_cstr(vlstr *vlstr_p, const char *cstr) {
    vlstr_init(vlstr_p, strlen(cstr));
    vlstr_append_cstr(vlstr_p, cstr);
}

/*!
 * Returns the vlstr as a C string (i.e. a char*)
 * @param vlstr_p
 * @return The vlstr as a char*.
 * Const because it's actually a pointer to the vlstr's internal array, and modifying that would be bad.
 */
const char *vlstr_as_cstr(const vlstr *vlstr_p) {
    return vlstr_p->arr;
}

size_t vlstr_len(const vlstr *vlstr_p) {
    return vlstr_p->n_elements - 1; // minus 1 to ignore the null byte
}

void vlstr_append(vlstr *dest, const vlstr *to_append) {
    vlstr_append_sized_cstr(dest, to_append->arr, to_append->n_elements);
}

void vlstr_append_cstr(vlstr *dest, const char *to_append) {
    vlstr_append_sized_cstr(dest, to_append, strlen(to_append));
}

void vlstr_append_sized_cstr(vlstr *dest, const char *to_append, size_t to_append_len) {
    if (dest->capacity < dest->n_elements + to_append_len) {
        char_vec_expand(dest, to_append_len);
    }
    // the -1 overwrites the old null termination
    memcpy(dest->arr + dest->n_elements - 1, to_append, to_append_len);
    dest->arr[dest->n_elements] = '\0';
    dest->n_elements += to_append_len;
}

void vlstr_append_char(vlstr *dest, const char chr) {
    char_vec_pop(dest);
    char_vec_append(dest, chr);
    char_vec_append(dest, '\0');
}

//bool vlstr_contains_char(vlstr *vlstr_p, const char chr) {
//    return char_vec_contains(vlstr_p, chr);
//}
