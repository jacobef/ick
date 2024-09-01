#ifndef ICK_DATA_STRUCTURES_MAP_H
#define ICK_DATA_STRUCTURES_MAP_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "debug/malloc.h"
#include "debug/reminder.h"

#define NODE_T(_key_t, _value_t) struct _key_t##_##_value_t##_node

#define DEFINE_MAP_TYPE(_key_t, _value_t)                                  \
    struct _key_t##_##_value_t##_node {                                    \
        _key_t key;                                                        \
        _value_t value;                                                    \
        struct _key_t##_##_value_t##_node *next;                           \
    };                                                                     \
    typedef struct _key_t##_##_value_t##_map {                             \
            NODE_T(_key_t, _value_t) **buckets;                            \
            size_t n_buckets;                                              \
            size_t n_elements;                                             \
            size_t (*hash_func)(const _key_t key, size_t n_buckets);       \
            bool (*keys_equal_func)(const _key_t key1, const _key_t key2); \
    } _key_t##_##_value_t##_map;

#define DEFINE_MAP_NEW_FUNCTION(_key_t, _value_t, _hash_func, _keys_equal_func)        \
    __attribute__((unused))                                                            \
    static _key_t##_##_value_t##_map _key_t##_##_value_t##_map_new(size_t n_buckets) { \
        if (n_buckets == 0) n_buckets = 1;                                             \
        _key_t##_##_value_t##_map map = {                                              \
            .hash_func = (_hash_func),                                                 \
            .keys_equal_func = (_keys_equal_func),                                     \
            .buckets = MALLOC(n_buckets * sizeof(NODE_T(_key_t, _value_t) *)),         \
            .n_buckets = n_buckets,                                                    \
            .n_elements = 0                                                            \
        };                                                                             \
        for (size_t i = 0; i < n_buckets; i++) {                                       \
            map.buckets[i] = NULL;                                                     \
        }                                                                              \
        REMEMBER("free " #_key_t " to " #_value_t " map internals");                   \
        return map;                                                                    \
    }

#define DEFINE_MAP_ADD_UNCHECKED_NO_EXPAND_FUNCTION(_key_t, _value_t)                                                                               \
    __attribute__((unused))                                                                                                                         \
    static void _key_t##_##_value_t##_map_add_unchecked_no_expand(_key_t##_##_value_t##_map *const map_p, const _key_t key, const _value_t value) { \
        const size_t entry_index = map_p->hash_func(key, map_p->n_buckets);                                                                         \
        NODE_T(_key_t, _value_t) *node = map_p->buckets[entry_index];                                                                               \
        if (node == NULL) {                                                                                                                         \
            map_p->buckets[entry_index] = MALLOC(sizeof(NODE_T(_key_t, _value_t)));                                                                 \
            *map_p->buckets[entry_index] = (NODE_T(_key_t, _value_t)) {                                                                             \
                .key = key,                                                                                                                         \
                .value = value,                                                                                                                     \
                .next = NULL                                                                                                                        \
            };                                                                                                                                      \
            return;                                                                                                                                 \
        }                                                                                                                                           \
        while (node->next != NULL) {                                                                                                                \
            node = node->next;                                                                                                                      \
        }                                                                                                                                           \
        node->next = MALLOC(sizeof(NODE_T(_key_t, _value_t)));                                                                                      \
        *node->next = (NODE_T(_key_t, _value_t)) {                                                                                                  \
            .key = key,                                                                                                                             \
            .value = value,                                                                                                                         \
            .next = NULL                                                                                                                            \
        };                                                                                                                                          \
        map_p->n_elements++;                                                                                                                        \
    }

#define DEFINE_MAP_EXPAND_FUNCTION(_key_t, _value_t)                                                                   \
    __attribute__((unused))                                                                                            \
    static void _key_t##_##_value_t##_map_expand(_key_t##_##_value_t##_map *const map_p, const size_t new_n_buckets) { \
        NODE_T(_key_t, _value_t) **const old_buckets = map_p->buckets;                                                 \
        const size_t old_n_buckets = map_p->n_buckets;                                                                 \
        map_p->buckets = MALLOC(new_n_buckets * sizeof(NODE_T(_key_t, _value_t) *));                                   \
        for (size_t i = 0; i < new_n_buckets; i++) {                                                                   \
            map_p->buckets[i] = NULL;                                                                                  \
        }                                                                                                              \
        map_p->n_buckets = new_n_buckets;                                                                              \
        map_p->n_elements = 0;                                                                                         \
        for (size_t i = 0; i < old_n_buckets; i++) {                                                                   \
            NODE_T(_key_t, _value_t) *node = old_buckets[i];                                                           \
            while (node != NULL) {                                                                                     \
                _key_t##_##_value_t##_map_add_unchecked_no_expand(map_p, node->key, node->value);                      \
                node = node->next;                                                                                     \
            }                                                                                                          \
        }                                                                                                              \
        FREE(old_buckets);                                                                                             \
    }

#define DEFINE_MAP_GET_FUNCTION(_key_t, _value_t)                                                                   \
    __attribute__((unused))                                                                                         \
    static _value_t _key_t##_##_value_t##_map_get(const _key_t##_##_value_t##_map *const map_p, const _key_t key) { \
        const size_t entry_index = map_p->hash_func(key, map_p->n_buckets);                                         \
        NODE_T(_key_t, _value_t) *node = map_p->buckets[entry_index];                                               \
        while (node != NULL) {                                                                                      \
            if (map_p->keys_equal_func(node->key, key)) {                                                           \
                return node->value;                                                                                 \
            }                                                                                                       \
            node = node->next;                                                                                      \
        }                                                                                                           \
        fprintf(stderr, "attempted to get a map value that doesn't exist\n");                                       \
        exit(1);                                                                                                    \
    }

#define DEFINE_MAP_CONTAINS_FUNCTION(_key_t, _value_t)                                                               \
    __attribute__((unused))                                                                                          \
    static bool _key_t##_##_value_t##_map_contains(const _key_t##_##_value_t##_map *const map_p, const _key_t key) { \
        const size_t entry_index = map_p->hash_func(key, map_p->n_buckets);                                          \
        NODE_T(_key_t, _value_t) *node = map_p->buckets[entry_index];                                                \
        while (node != NULL) {                                                                                       \
            if (map_p->keys_equal_func(node->key, key)) {                                                            \
                return true;                                                                                         \
            }                                                                                                        \
            node = node->next;                                                                                       \
        }                                                                                                            \
        return false;                                                                                                \
    }

#define DEFINE_MAP_ADD_FUNCTION(_key_t, _value_t)                                                                               \
    __attribute__((unused))                                                                                                     \
    static void _key_t##_##_value_t##_map_add(_key_t##_##_value_t##_map *const map_p, const _key_t key, const _value_t value) { \
        if (_key_t##_##_value_t##_map_contains(map_p, key)) {                                                                   \
            fprintf(stderr, "attempted to add key that already exists in the map\n");                                           \
            exit(1);                                                                                                            \
        }                                                                                                                       \
        if (map_p->n_elements >= map_p->n_buckets) {                                                                            \
            _key_t##_##_value_t##_map_expand(map_p, map_p->n_buckets * 2);                                                      \
        }                                                                                                                       \
        _key_t##_##_value_t##_map_add_unchecked_no_expand(map_p, key, value);                                                   \
    }

#define DEFINE_MAP_REMOVE_FUNCTION(_key_t, _value_t)                                                         \
    __attribute__((unused))                                                                                  \
    static bool _key_t##_##_value_t##_map_remove(_key_t##_##_value_t##_map *const map_p, const _key_t key) { \
        const size_t bucket_index = map_p->hash_func(key, map_p->n_buckets);                                 \
        NODE_T(_key_t, _value_t) **node_ptr = &map_p->buckets[bucket_index];                                 \
        while (*node_ptr != NULL) {                                                                          \
            NODE_T(_key_t, _value_t) *const current_node = *node_ptr;                                        \
            if (map_p->keys_equal_func(current_node->key, key)) {                                            \
                *node_ptr = current_node->next;                                                              \
                FREE(current_node);                                                                          \
                map_p->n_elements--;                                                                         \
                return true;                                                                                 \
            }                                                                                                \
            node_ptr = &current_node->next;                                                                  \
        }                                                                                                    \
        return false;                                                                                        \
    }

#define DEFINE_MAP_FREE_INTERNALS_FUNCTION(_key_t, _value_t)                                       \
    __attribute__((unused))                                                                        \
    static void _key_t##_##_value_t##_map_free_internals(_key_t##_##_value_t##_map *const map_p) { \
        for (size_t i = 0; i < map_p->n_buckets; i++) {                                            \
            NODE_T(_key_t, _value_t) *node = map_p->buckets[i];                                    \
            while (node != NULL) {                                                                 \
                NODE_T(_key_t, _value_t) *const node_next = node->next;                            \
                FREE(node);                                                                        \
                node = node_next;                                                                  \
            }                                                                                      \
        }                                                                                          \
        REMEMBERED_TO("free " #_key_t " to " #_value_t " map internals");                          \
    }

#define DEFINE_MAP_TYPE_AND_FUNCTIONS(_key_t, _value_t, _hash_func, _keys_equal_func) \
    DEFINE_MAP_TYPE(_key_t, _value_t)                                                 \
    DEFINE_MAP_NEW_FUNCTION(_key_t, _value_t, _hash_func, _keys_equal_func)           \
    DEFINE_MAP_ADD_UNCHECKED_NO_EXPAND_FUNCTION(_key_t, _value_t)                     \
    DEFINE_MAP_EXPAND_FUNCTION(_key_t, _value_t)                                      \
    DEFINE_MAP_GET_FUNCTION(_key_t, _value_t)                                         \
    DEFINE_MAP_CONTAINS_FUNCTION(_key_t, _value_t)                                    \
    DEFINE_MAP_ADD_FUNCTION(_key_t, _value_t)                                         \
    DEFINE_MAP_REMOVE_FUNCTION(_key_t, _value_t)                                      \
    DEFINE_MAP_FREE_INTERNALS_FUNCTION(_key_t, _value_t)

#endif // ICK_DATA_STRUCTURES_MAP_H
