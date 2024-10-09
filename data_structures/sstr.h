#ifndef SSTR_H
#define SSTR_H

#include <stdbool.h>
#include "vector.h"

typedef unsigned char uchar;
DEFINE_VEC_TYPE_AND_FUNCTIONS(uchar)
typedef uchar_harr sstr;

bool sstr_cstr_eq(sstr view, const char *cstr);
bool sstrs_eq(sstr t1, sstr t2);
sstr slice(sstr str, size_t begin, size_t end);

#define SSTR_FROM_LITERAL(literal) (sstr){ .data = (unsigned char*)strdup(literal), .len = sizeof(literal)-1 }


#endif //SSTR_H
