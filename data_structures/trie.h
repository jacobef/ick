#ifndef ICK_TRIE_H
#define ICK_TRIE_H

#include <stdint.h>
#include <stdbool.h>

struct trie {
    struct trie *children;
    uint_fast8_t n_children;
    bool match;
    char val;
};

struct trie *trie_get_child(struct trie *trie, char c);

#endif //ICK_TRIE_H
