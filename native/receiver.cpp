#include "receiver.h"

#include "constants.h"
#include "packeter.h"
#include "sync.h"
#include "stream.h"

#include <cassert>
#include <iostream>

// NOTE(alex): There's lots of copying here. Consider using pointers to 
// vectors instead of direct vectors all the time, or avoiding nested
// vectors.

using namespace std;

static const int WAITING_SYNC = 0;
static const int RECEIVING_MESSAGE = 1;
static const int SUB_PROGRESS_PARTS = 5;

Receiver::Receiver(Config *cfg, Stream<float>& source, Sync *sync, Packeter *packeter) :
    state(WAITING_SYNC),
    cfg(cfg),
    source(source),
    sync(sync),
    packeter(packeter) {
  progress = 0;
  subProgress = 0;
}

int Receiver::receiveAudio() {
  while (true) {
    cerr << "while true" << endl;
    // If not yet synced, try
    if (state == WAITING_SYNC) {
      //cerr << "waiting sync" << endl;
      bool synced = sync->sync();

      if (synced) {
        cerr << "synced " << endl;
        state = RECEIVING_MESSAGE;
        progress = 1;
        assert(partialMessage.size() == 0);
      } else {
        //cerr << "didn't sync" << endl;
        break;
      }
    }

    // If synced, start/continue decoding message
    if (state == RECEIVING_MESSAGE) {
      cerr << "Receving message with " << source.size() << " samples" << endl;
      int chunkChips = packeter->chunkChips();
      int chunkSamples = chunkChips * cfg->chipSamples;
      bool messageDone = false;
      int bufferedSamples = source.size();
      subProgress = bufferedSamples * SUB_PROGRESS_PARTS / chunkSamples;
      if (bufferedSamples >= chunkSamples) {
        progress += SUB_PROGRESS_PARTS;
        subProgress = 0;
        vector<vector<float> > chips;
        receiveChips(chunkChips, chips);
        cerr << "Recieved " << chunkChips << " chips, remaining samples " << source.size() << endl;
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
          progress = 0;
        }
      } else {
        cerr << "Need more samples than " << source.size() << endl;
        break;
      }
    }
  }

  return progress + subProgress;
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

void Receiver::receiveChips(int numChips, vector<vector<float> > &target) {
  assert(source.size() >= cfg->chipSamples * numChips);
  target.reserve(target.size() + numChips);
  vector<float>::iterator sampleItr = source.begin();

  for (int c = 0; c < numChips; ++c) {
    // Fouriate
    float *firstSample = source.raw() + (sampleItr - source.begin());
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
  source.consumeUntil(sampleItr);
}

