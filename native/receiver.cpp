#include "receiver.h"

#include "constants.h"
#include "packeter.h"
#include "sync.h"

#include <cassert>
#include <iostream>

// NOTE(alex): There's lots of copying here. Consider using pointers to 
// vectors instead of direct vectors all the time, or avoiding nested
// vectors.

using namespace std;

static const int WAITING_SYNC = 0;
static const int RECEIVING_MESSAGE = 1;

Receiver::Receiver(Config *cfg, Sync *sync, Packeter *packeter) :
    state(WAITING_SYNC),
    cfg(cfg),
    sync(sync),
    packeter(packeter) {
}

void Receiver::receiveAudio(vector<float> &samples) {
  buffer.insert(buffer.end(), samples.begin(), samples.end());
  vector<float>::iterator sampleItr = buffer.begin();

  while (true) {
    // If not yet synced, try
    if (state == WAITING_SYNC) {
      sampleItr = sync->receiveAudioAndSync(buffer);
      assert(sampleItr >= buffer.begin());
      assert(sampleItr <= buffer.end());

      if (sampleItr != buffer.end()) { // synced
        state = RECEIVING_MESSAGE;
        assert(partialMessage.size() == 0);
      } else {
        // Consumed all buffer
        buffer.clear();
        break;
      }
    }

    assert(sampleItr <= buffer.end());
    // If synced, start/continue decoding message
    if (state == RECEIVING_MESSAGE) {
      int chunkChips = packeter->chunkChips();
      int chunkSamples = chunkChips * cfg->chipSamples;
      bool messageDone = false;
      if ((buffer.end() - sampleItr) >= chunkSamples) {
        vector<vector<float> > chips;
        sampleItr = receiveChips(buffer, sampleItr, chunkChips, chips);
        assert(sampleItr <= buffer.end());
        assert(sampleItr > buffer.begin());
        int result = packeter->decodePartial(chips, partialMessage);
        if (result < 0) { // Failed
          messageDone = true;
        } else if (result == 0) { // Complete
          messages.push(partialMessage);
          messageDone = true;
        } else {
          // Need more chunks
        }

        if (messageDone) {
          partialMessage.clear();
          state = WAITING_SYNC;
        }
      } else {
        break;
      }
    }

    assert(sampleItr <= buffer.end());
    buffer.erase(buffer.begin(), sampleItr);
    sampleItr = buffer.begin();
  }
}

bool Receiver::messageAvailable() {
  return !messages.empty();
}

int Receiver::takeMessage(char *buffer, int bufferCapacity) {
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

// Private

vector<float>::iterator Receiver::receiveChips(vector<float> &samples, 
    vector<float>::iterator firstSample, int numChips, vector<vector<float> > &target) {
  assert(samples.end() - firstSample >= cfg->chipSamples * numChips);
  target.reserve(target.size() + numChips);
  vector<float>::iterator sampleItr = firstSample;

  for (int c = 0; c < numChips; ++c) {
    // Fouriate
    float *firstSample = samples.data() + (sampleItr - samples.begin());
    Spectrum spec(firstSample, SAMPLE_RATE, cfg->chipSamples);

    // Process amplitudes
    target.push_back(vector<float>());
    vector<float>& chip = target.back();
    for (int i = cfg->baseBucket; i < cfg->baseBucket + (cfg->numChannels * cfg->channelSpacing);
        i += cfg->channelSpacing) {
      chip.push_back(spec.amplitude(i));
    }

    sampleItr += cfg->chipSamples;
  }
  return sampleItr;
}

