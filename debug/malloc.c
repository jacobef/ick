#include "debug/malloc.h"
#include "debug/reminder.h"
#include "driver/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(const size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        driver_error("malloc failed");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, const size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        driver_error("realloc failed");
        exit(1);
    }
    return new_ptr;
}

#ifdef DEBUG

void *debug_malloc(size_t size, const char *file, const char *func, int line) {
    void *ptr = xmalloc(size);
    char reminder_message[MAX_REMINDER_MESSAGE_LENGTH];
    snprintf(reminder_message, MAX_REMINDER_MESSAGE_LENGTH, "Free allocation at %p", ptr);
    add_reminder(reminder_message, file, func, line);
    return ptr;
}

void *debug_realloc(void *ptr, size_t size, const char *file, const char *func, int line) {
    if (!ptr) return debug_malloc(size, file, func, line);

    char reminder_message[MAX_REMINDER_MESSAGE_LENGTH];
    snprintf(reminder_message, MAX_REMINDER_MESSAGE_LENGTH, "Free allocation at %p", ptr);
    remove_reminder(reminder_message, file, func, line);
    void *new_ptr = xrealloc(ptr, size);
    if (new_ptr) {
        snprintf(reminder_message, MAX_REMINDER_MESSAGE_LENGTH, "Free allocation at %p", new_ptr);
        add_reminder(reminder_message, file, func, line);
    }
    return new_ptr;
}

void debug_free(void *ptr, const char *file, const char *func, int line) {
    if (!ptr) return;

    char reminder_message[MAX_REMINDER_MESSAGE_LENGTH];
    snprintf(reminder_message, MAX_REMINDER_MESSAGE_LENGTH, "Free allocation at %p", ptr);
    remove_reminder(reminder_message, file, func, line);
    free(ptr);
}
#endif
