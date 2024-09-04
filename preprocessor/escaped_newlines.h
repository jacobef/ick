#ifndef TEST_ESCAPED_NEWLINES_H
#define TEST_ESCAPED_NEWLINES_H

#include "data_structures/sized_str.h"
#include "data_structures/vector.h"

struct escaped_newlines_replacement_info {
    struct str_view result;
    size_t_vec backslash_locations;
};

struct escaped_newlines_replacement_info rm_escaped_newlines(struct str_view in);

#endif //TEST_ESCAPED_NEWLINES_H
