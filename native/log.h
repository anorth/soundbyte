#ifndef _LOG_H_
#define _LOG_H_

#include <cstdarg>

typedef enum {
    LOG_VERBOSE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} LOG_PRIORITY;

void setPriority(LOG_PRIORITY pri);

/** Logs a simple string. */
void ll(int pri, const char *tag, const char *text);

/** Logs a formatted string. */
void ll(int pri, const char *tag,  const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 3, 4)))
#endif
    ;

/** Logs a formatted string with va list */
void ll(int pri, const char *tag, const char *fmt, va_list ap);

#endif
