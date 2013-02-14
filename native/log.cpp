// Logging for development environment

#include "log.h"

#include <cstdio>
#include <cstdarg>

static LOG_PRIORITY currentPriority = LOG_INFO;

static void logTag(int pri, const char* tag) {
  fprintf(stderr, "%d ", pri);
  fputs(tag, stderr);
  fputs(": ", stderr);
}

void setPriority(LOG_PRIORITY pri) {
  currentPriority = pri;
}

void ll(int pri, const char *tag, const char *text) {
  if (pri >= currentPriority) {
    logTag(pri, tag);
    fputs(text, stderr);
    fputs("\n", stderr);
  }
}

void ll(int pri, const char *tag,  const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  ll(pri, tag, fmt, ap);
  va_end(ap);
}

void ll(int pri, const char *tag, const char *fmt, va_list ap) {
  if (pri >= currentPriority) {
    logTag(pri, tag);
    vfprintf(stderr, fmt, ap);
    fputs("\n", stderr);
  }
}

