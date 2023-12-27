#ifndef ICK_DATA_STRUCTURES_MAP_H
#define ICK_DATA_STRUCTURES_MAP_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

size_t hash_cstr(char *cstr, size_t capacity);
size_t hash_char(char chr, size_t capacity);
bool cstrs_equal(char *str1, char *str2);
bool chars_equal(char c1, char c2);

#define DEFINE_MAP_TYPE(_key_t, _value_t)                     \
    typedef struct _key_t##_##_value_t##_map {                    \
            _key_t *keys;                                         \
            _value_t *values;                                     \
            bool *indices_free;                                   \
            size_t n_indices_free;                              \
            size_t capacity;                                      \
            /* hash_func's min val should always be 0 */        \
            size_t (*hash_func)(_key_t key, size_t capacity);   \
            bool (*keys_equal_func)(_key_t key1, _key_t key2);  \
    } _key_t##_##_value_t##_map;

#define DEFINE_MAP_INIT_FUNCTION(_key_t, _value_t, _hash_func, _keys_equal_func)                     \
    static void _key_t##_##_value_t##_map##_init(_key_t##_##_value_t##_map *map_p, size_t capacity) { \
        if (capacity == 0) capacity = 1;                                                                     \
        map_p->hash_func = (_hash_func);                                                                 \
        map_p->keys_equal_func = (_keys_equal_func);                                                     \
        map_p->keys = MALLOC(capacity*sizeof(_key_t));                                                       \
        map_p->values = MALLOC(capacity*sizeof(_value_t));                                                   \
        map_p->indices_free = MALLOC(capacity*sizeof(bool));                                                 \
        map_p->n_indices_free = capacity;                                                                  \
        for (size_t i = 0; i < capacity; i++) {                                                              \
            map_p->indices_free[i] = true;                                                                   \
        }                                                                                                    \
        map_p->capacity = capacity;                                                                  \
        REMEMBER("free " #_key_t " to " #_value_t " map internals");                                           \
    }

// TODO change to chainnnn stuff
#define TRY_NEW_ENTRY_INDEX(_map_p, _entry_index_guess) \
    _entry_index_guess = (_entry_index_guess) % (_map_p)->capacity - 1

#define MAP_CALL_HASH_FUNC(_map_p, _key) (_map_p)->hash_func((_key), (_map_p)->capacity)
#define MAP_CALL_KEYS_EQUAL(_map_p, _key1, _key2) (_map_p)->keys_equal_func((_key1), (_key2))

#define DEFINE_MAP_APPEND_NO_EXPAND_FUNCTION(_key_t, _value_t)                                                                  \
    static void _key_t##_##_value_t##_map##_append_no_expand(_key_t##_##_value_t##_map *map_p, _key_t key, _value_t value) { \
        size_t entry_index = MAP_CALL_HASH_FUNC(map_p, key);                                                                        \
        while (map_p->indices_free[entry_index] == false) {                                                                         \
            TRY_NEW_ENTRY_INDEX(map_p, entry_index);                                                                                \
        }                                                                                                                           \
        map_p->keys[entry_index] = key;                                                                                             \
        map_p->values[entry_index] = value;                                                                                         \
        map_p->indices_free[entry_index] = false;                                                                                   \
        map_p->n_indices_free--;                                                                                                  \
    }

#define DEFINE_MAP_EXPAND_FUNCTION(_key_t, _value_t)                                                            \
    static void _key_t##_##_value_t##_map##_expand(_key_t##_##_value_t##_map *map_p, size_t new_capacity)  { \
        _key_t *old_keys = map_p->keys;                                                                             \
        _value_t *old_values = map_p->values;                                                                       \
        bool *old_indices_free = map_p->indices_free;                                                               \
        size_t old_capacity = map_p->capacity;                                                                      \
                                                                                                                    \
        map_p->keys = MALLOC(new_capacity * sizeof(_key_t));                                                        \
        map_p->values = MALLOC(new_capacity * sizeof(_value_t));                                                    \
        map_p->capacity = new_capacity;                                                                             \
                                                                                                                    \
        map_p->indices_free = MALLOC(new_capacity*sizeof(bool));                                                    \
        map_p->n_indices_free = new_capacity;                                                                     \
        for (size_t i = 0; i < new_capacity; i++) {                                                                 \
            map_p->indices_free[i] = true;                                                                          \
        }                                                                                                           \
        for (size_t i = 0; i < old_capacity; i++) {                                                                 \
            if (old_indices_free[i] == false)                                                                       \
                _key_t##_##_value_t##_map##_append_no_expand(map_p, old_keys[i], old_values[i]);                    \
        }                                                                                                           \
                                                                                                                    \
        FREE(old_indices_free);                                                                                     \
        FREE(old_keys);                                                                                             \
        FREE(old_values);                                                                                           \
    }

#define DEFINE_MAP_APPEND_FUNCTION(_key_t, _value_t)                                                                  \
    static void _key_t##_##_value_t##_map##_append(_key_t##_##_value_t##_map *map_p, _key_t key, _value_t value) { \
        if (map_p->n_indices_free == 0) {                                                                               \
            _key_t##_##_value_t##_map##_expand(map_p, map_p->capacity*2);                                                 \
        }                                                                                                                 \
        _key_t##_##_value_t##_map##_append_no_expand(map_p, key, value);                                                  \
    }

#define DEFINE_MAP_GET_FUNCTION(_key_t, _value_t)                                                                    \
    static _value_t _key_t##_##_value_t##_map##_get(_key_t##_##_value_t##_map *map_p, _key_t key) {               \
        size_t entry_index = MAP_CALL_HASH_FUNC(map_p, key);                                                             \
        while (map_p->indices_free[entry_index] == true || !MAP_CALL_KEYS_EQUAL(map_p, map_p->keys[entry_index], key)) { \
            TRY_NEW_ENTRY_INDEX(map_p, entry_index);                                                                     \
        }                                                                                                                \
        return map_p->values[entry_index];                                                                               \
    }

#define DEFINE_MAP_FREE_INTERNALS_FUNCTION(_key_t, _value_t)                                      \
    static void _key_t##_##_value_t##_map##_free_internals(_key_t##_##_value_t##_map *map_p) { \
        FREE(map_p->keys);                                                                            \
        FREE(map_p->values);                                                                          \
        FREE(map_p->indices_free);                                                                \
        REMEMBERED_TO("free " #_key_t " to " #_value_t " map internals");                            \                                                                                   \
    }

#define DEFINE_MAP_TYPE_AND_FUNCTIONS(_key_t, _value_t, _hash_func, _keys_equal_func) \
    DEFINE_MAP_TYPE(_key_t, _value_t)                                                     \
    DEFINE_MAP_INIT_FUNCTION(_key_t, _value_t, _hash_func, _keys_equal_func)          \
    DEFINE_MAP_APPEND_NO_EXPAND_FUNCTION(_key_t, _value_t)                                \
    DEFINE_MAP_EXPAND_FUNCTION(_key_t, _value_t)                                          \
    DEFINE_MAP_APPEND_FUNCTION(_key_t, _value_t)                                          \
    DEFINE_MAP_GET_FUNCTION(_key_t, _value_t)                                             \
    DEFINE_MAP_FREE_INTERNALS_FUNCTION(_key_t, _value_t)

#endif //ICK_DATA_STRUCTURES_MAP_H
