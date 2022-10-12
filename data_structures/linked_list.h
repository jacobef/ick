#ifndef ICK_LINKED_LIST_H
#define ICK_LINKED_LIST_H

#define DEFINE_LINKED_LIST(_type)     \
struct _type##_linked_list {          \
    struct _type##_linked_list *next; \
    _type val;                        \
};


#endif //ICK_LINKED_LIST_H
