#include "reminder.h"
#include <stdio.h>
#include <string.h>

#ifdef DEBUG

struct reminder {
    char message[MAX_REMINDER_MESSAGE_LENGTH+1];
    const char *file;
    const char *func;
    int line;
};

static struct reminder reminders[MAX_REMINDERS];
static size_t reminder_count = 0;

void add_reminder(const char *message, const char *file, const char *func, int line) {
    fprintf(stderr, "Adding reminder to '%s' from function %s at %s:%d...\n", message, func, file, line);

    if (reminder_count < MAX_REMINDERS) {
        snprintf(reminders[reminder_count].message, MAX_REMINDER_MESSAGE_LENGTH, "%s", message);
        reminders[reminder_count].file = file;
        reminders[reminder_count].func = func;
        reminders[reminder_count].line = line;
        ++reminder_count;
    }
}

void remove_reminder(const char *message, const char *file, const char *func, int line) {
    fprintf(stderr, "Removing reminder to '%s' from function %s at %s:%d...\n", message, func, file, line);

    for (size_t i = 0; i < reminder_count; ++i) {
        if (strcmp(reminders[i].message, message) == 0) {
            for (size_t j = i; j < reminder_count - 1; ++j) {
                reminders[j] = reminders[j + 1];
            }
            --reminder_count;
            return;
        }
    }
    fprintf(stderr, "Reminder to '%s' from function %s at %s:%d was not found.\n", message, func, file, line);
}

void check_reminders(void) {
    if (reminder_count > 0) {
        fprintf(stderr, "Unresolved reminders:\n");
        for (size_t i = 0; i < reminder_count; ++i) {
            fprintf(stderr, "%s (in %s at %s:%d)\n", reminders[i].message, reminders[i].func, reminders[i].file, reminders[i].line);
        }
    }
}
#endif
