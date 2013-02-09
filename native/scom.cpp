#include "scom.h"

const char *HELLO = "Hello from C++";

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  return 0;
}

void decodeAudio(char *buffer, int buflen) {
  // Not yet implemented.
}

bool messageAvailable() {
  return false;
}

int takeMessage(char *buffer, int bufferCapacity) {
  return -1; // No message available.
}
