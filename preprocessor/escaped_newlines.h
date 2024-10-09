#ifndef TEST_ESCAPED_NEWLINES_H
#define TEST_ESCAPED_NEWLINES_H

#include "data_structures/sstr.h"
#include "data_structures/vector.h"

struct escaped_newlines_replacement_info {
    sstr result;
    size_t_harr backslash_locations;
};

struct escaped_newlines_replacement_info rm_escaped_newlines(sstr in);

#endif //TEST_ESCAPED_NEWLINES_H
