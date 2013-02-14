// Logging for development environment

#include "log.h"
#include "android/log.h"

void setPriority(LOG_PRIORITY pri) {
  // n/a
}

void ll(int pri, const char *tag, const char *text) {
  __android_log_write(pri, tag, text);
}

void ll(int pri, const char *tag,  const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  ll(pri, tag, fmt, ap);
  va_end(ap);
}

void ll(int pri, const char *tag, const char *fmt, va_list ap) {
  __android_log_vprint(pri, tag, fmt, ap);
}


