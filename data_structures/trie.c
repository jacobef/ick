#include "trie.h"
#include <stddef.h>

struct trie *trie_get_child(const struct trie *const trie, const unsigned char c) {
    for (size_t i = 0; i < trie->n_children; i++) {
        if (trie->children[i].val == c) return &trie->children[i];
    }
    return NULL;
}
