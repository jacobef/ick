#ifndef ICK_RESULT_H
#define ICK_RESULT_H
#include <stdio.h>
#include <stdlib.h>

enum result_validity { OK, ERR };

#define DEFINE_RESULT_TYPE(_type) \
struct result_##_type {           \
    enum result_validity tag;     \
    union {                       \
        _type ok;                 \
        char *err;                \
    } val;                        \
};


#define DEFINE_UNWRAP(_type)                                    \
__attribute__((unused))                                         \
static _type unwrap_##_type(struct result_##_type result) {     \
    if (result.tag == OK) return result.val.ok;                 \
    fprintf(stderr, "unwrapped error (%s)", result.val.err);    \
    exit(1);                                                    \
}

#define OK(_type, _val) (struct result_##_type) { .tag=OK, .val.ok=_val }
#define ERR(_type, _msg) (struct result_##_type) { .tag=ERR, .val.err=_msg }

#define IS_OK(_result) (_result.tag == OK)
#define IS_ERR(_result) (_result.tag == ERR)

#define AND_THEN(_result, _func) (_result.tag == OK ? _func(_result.val.ok) : _result)


typedef void *void_p;
DEFINE_RESULT_TYPE(void_p)
DEFINE_UNWRAP(void_p)

__attribute__((unused))
static struct result_void_p rmalloc(const size_t size) {
    void *res = malloc(size);
    if (res != NULL) {
        return OK(void_p, res);
    } else {
        return ERR(void_p, "malloc failed");
    }
}

__attribute__((unused))
static struct result_void_p do_something(void *ptr) {
    if (ptr == NULL) return ERR(void_p , "it was null I guess");
    else return OK(void_p, ptr);
}


#endif //ICK_RESULT_H
