#include "scom.h"

#include "audio.h"
#include "constants.h"
#include "decoder.h"

#include <cassert>
#include <vector>

using namespace std;

const char *HELLO = "Hello from C++";

static Decoder decoder;

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  return 0;
}

void decodeAudio(char *buffer, int buflen) {
  std::vector<float> samples;
  decodePcm16(buffer, buflen, samples);
  decoder.receiveAudio(samples);
}

bool messageAvailable() {
  return decoder.messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return decoder.takeMessage(buffer, bufferCapacity);
}

///// Private /////



