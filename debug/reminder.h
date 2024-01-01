#ifndef ICK_REMINDER_H
#define ICK_REMINDER_H

#ifdef DEBUG
#define MAX_REMINDERS 100
#define MAX_REMINDER_MESSAGE_LENGTH 256

void add_reminder(const char *message, const char *file, const char *func, int line);
void remove_reminder(const char *message, const char *file, const char *func, int line);
void check_reminders(void);

#define REMEMBER(msg) add_reminder(msg, __FILE__, __func__, __LINE__)
#define REMEMBERED_TO(msg) remove_reminder(msg, __FILE__, __func__, __LINE__)
#else
#define REMEMBER(msg)
#define REMEMBERED_TO(msg)
#endif

#endif //ICK_REMINDER_H
