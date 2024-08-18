#ifndef ICK_MALLOC_H
#define ICK_MALLOC_H
#include <stddef.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

#ifdef DEBUG

void *debug_malloc(size_t size, const char *file, const char *func, int line);
void *debug_realloc(void *ptr, size_t size, const char *file, const char *func, int line);
void debug_free(void *ptr, const char *file, const char *func, int line);

#define MALLOC(size) debug_malloc(size, __FILE__, __func__, __LINE__)
#define REALLOC(ptr, size) debug_realloc(ptr, size, __FILE__, __func__, __LINE__)
#define FREE(ptr) _Pragma("clang diagnostic push") \
_Pragma("clang diagnostic ignored \"-Wcast-qual\"") \
debug_free((void *)(ptr), __FILE__, __func__, __LINE__); \
_Pragma("clang diagnostic pop")

#else

#define MALLOC(size) xmalloc(size)
#define REALLOC(ptr, size) xrealloc(ptr, size)
#define FREE(ptr) _Pragma("clang diagnostic push") \
_Pragma("clang diagnostic ignored \"-Wcast-qual\"") \
do { free((void*)(ptr)); } while (0) \
_Pragma("clang diagnostic pop")

#endif

#endif //ICK_MALLOC_H
