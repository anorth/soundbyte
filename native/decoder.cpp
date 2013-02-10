#include "decoder.h"

#include "constants.h"

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 10;

Decoder::Decoder() {
}

void Decoder::receiveAudio(std::vector<float> &samples) {
  // Append samples to buffer
  buffer.resize(buffer.size() + samples.size());
  std::copy(samples.begin(), samples.end(), buffer.end());

  // Scan buffer for sync

  // Discard head of buffer if appropriate
  while (buffer.size() > MAX_BUFFER_SAMPLES) {
    buffer.pop_front(); // slow?
  }
}

bool Decoder::messageAvailable() {
  return !messages.empty();
}

int Decoder::takeMessage(char *buffer, int bufferCapacity) {
  return -1; // No message available.
}


