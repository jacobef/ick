#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser.h"

pp_token_harr preprocess_full_file_tree(const struct earley_rule *root);
static pp_token_harr preprocess_tree(struct earley_rule group_opt_rule);

#endif //PREPROCESSOR_H
