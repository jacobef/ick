#ifndef ICK_DIAGNOSTICS_H
#define ICK_DIAGNOSTICS_H

extern char *ick_progname;

void driver_warning(const char *msg_fmt, ...);
void driver_error(const char *msg_fmt, ...);

#endif //ICK_DIAGNOSTICS_H
