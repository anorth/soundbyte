#include <stdio.h>
#include <execinfo.h>
#include <csignal>
#include <stdlib.h>

void signal_handler(int sig) {
  void *array[100];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 100);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, 
      //STDERR_FILENO
      STDOUT_FILENO
      );
  exit(1);
}
