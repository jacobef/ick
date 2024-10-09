#ifndef SSTR_H
#define SSTR_H

#include <stdbool.h>
#include "vector.h"

typedef unsigned char uchar;
DEFINE_VEC_TYPE_AND_FUNCTIONS(uchar)
typedef uchar_harr sstr;

bool sstr_cstr_eq(const sstr view, const char *const cstr);
bool sstrs_eq(const sstr t1, const sstr t2);
#define SSTR_FROM_LITERAL(literal) (sstr){ .data = (unsigned char*)strdup(literal), .len = sizeof(literal)-1 }
// TODO add slice function

#endif //SSTR_H
