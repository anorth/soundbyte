//
//  SBTLog.c
//  Soundbyte
//
//  Created by Alex on 18/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#include <stdio.h>

#include <Scom/log.h>

static LOG_PRIORITY currentPriority = LOG_INFO;

//static void logTag(int pri, const char* tag) {
//  fprintf(stderr, "%d ", pri);
//  fputs(tag, stderr);
//  fputs(": ", stderr);
//}

void setPriority(LOG_PRIORITY pri) {
  currentPriority = pri;
}

void ll(int pri, const char *tag,  const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  ll(pri, tag, fmt, ap);
  va_end(ap);
}

void ll(int pri, const char *tag, const char *fmt, va_list ap) {
  if (pri >= currentPriority) {
    char newFmt[1000];
    snprintf(newFmt, sizeof(newFmt), "%d %s: %s", pri, tag, fmt);
    NSLogv([NSString stringWithUTF8String:newFmt], ap);
  }
}
