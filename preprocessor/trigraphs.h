#ifndef ICK_TRIGRAPHS_H
#define ICK_TRIGRAPHS_H

#include <stddef.h>
#include "data_structures/sized_str.h"
#include "data_structures/vector.h"

struct trigraph_replacement_info {
    struct str_view result;
    size_t_vec original_trigraph_locations;
};

struct trigraph_replacement_info replace_trigraphs(struct str_view in);

#endif //ICK_TRIGRAPHS_H
