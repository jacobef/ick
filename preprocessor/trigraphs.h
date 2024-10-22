#ifndef ICK_TRIGRAPHS_H
#define ICK_TRIGRAPHS_H

#include "data_structures/sstr.h"
#include "data_structures/vector.h"

struct trigraph_replacement_info {
    sstr result;
    size_t_harr original_trigraph_locations;
};

struct trigraph_replacement_info replace_trigraphs(sstr in);

#endif //ICK_TRIGRAPHS_H
