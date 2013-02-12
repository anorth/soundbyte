#include "decoder.h"

#include "constants.h"
#include "packeter.h"
#include "sync.h"

using namespace std;

static const int WAITING_SYNC = 0;
static const int RECEIVING_MESSAGE = 1;

Decoder::Decoder(Sync *sync, Packeter *packeter) :
    state(WAITING_SYNC),
    sync(sync),
    packeter(packeter) {
}

void Decoder::receiveAudio(vector<float> &samples) {
  vector<float>::iterator sampleItr;

  // If not yet synced, try
  if (state == WAITING_SYNC) {
    sampleItr = sync->receiveAudioAndSync(samples);
    if (sampleItr != samples.end()) {
      state = RECEIVING_MESSAGE;
    }
  }

  // If synced, start/continue decoding message
  if (state == RECEIVING_MESSAGE) {
    // Receive some message from sampleItr
    // If finished decoding message, enqueue it
    state = WAITING_SYNC;
  }
}

bool Decoder::messageAvailable() {
  return !messages.empty();
}

int Decoder::takeMessage(char *buffer, int bufferCapacity) {
  int ret = -1; // No message available
  if (messageAvailable()) {
    vector<char> &message = messages.front();
    if (bufferCapacity >= message.size()) {
      std::copy(message.begin(), message.end(), buffer);
      ret = message.size();
      messages.pop();
    } else {
      ret = 0;
    }
  } 
  return ret;
}


