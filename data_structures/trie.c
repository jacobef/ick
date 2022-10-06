#include "trie.h"
#include <stddef.h>

struct trie *trie_get_child(struct trie *trie, char c) {
    for (size_t i = 0; i < trie->n_children; i++) {
        if (trie->children[i].val == c) return &trie->children[i];
    }
    return NULL;
}
