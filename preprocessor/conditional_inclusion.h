#ifndef ICK_CONDITIONAL_INCLUSION_H
#define ICK_CONDITIONAL_INCLUSION_H

#include "preprocessor/macro_expansion.h"
#include "mappings/typedefs.h"

struct maybe_signed_intmax {
    union {
        target_intmax_t signd;
        target_uintmax_t unsignd;
    } val;
    bool is_signed;
};
struct earley_rule *eval_if_section(struct earley_rule if_section_rule);

#endif //ICK_CONDITIONAL_INCLUSION_H
