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

  // If not yet synced, try
  vector<float>::iterator sampleItr = samples.begin();
  if (state == WAITING_SYNC) {
    sampleItr = sync->receiveAudioAndSync(samples);
    if (sampleItr != samples.end()) {
      state = RECEIVING_MESSAGE;
      assert(decoded.size() == 0);

      // TODO: change code to use a pair of iters, or float*/int,
      // to avoid unnecessary copying.
      samples.erase(samples.begin(), sampleItr);
    }
  }

  // If synced, start/continue decoding message
  if (state == RECEIVING_MESSAGE) {
    receiveChips(samples);
    int numMessageSymbols = packeter->chunkChips();
    while (chips.size() >= numMessageSymbols) {
      vector<vector<float> > messageChips;
      takeChips(numMessageSymbols, messageChips);

      int result = packeter->decodePartial(messageChips, decoded);
      if (result < 0) {
        decoded.clear();
        state = WAITING_SYNC;
        break;
      } else if (result == 0) {
        messages.push(decoded);

        // TODO: factor out state transitions & cleanup, etc.
        decoded.clear();
        state = WAITING_SYNC; 
      }
    }
    
    // Receive some message from sampleItr
    // If finished decoding message, enqueue it
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

void Receiver::receiveChips(vector<float> &samples) {
  vector<float>::iterator sampleItr = samples.begin();
  // Finish off any partial chip.
  if (!partialChip.empty()) {
    int gap = cfg->chipSamples - partialChip.size();
    if (samples.size() < gap) {
      partialChip.insert(partialChip.end(), samples.begin(), samples.end());
      sampleItr = samples.end();
    } else {
      partialChip.insert(partialChip.end(), sampleItr, sampleItr + gap);
      sampleItr += gap;
      makeChip(partialChip, partialChip.begin());
      partialChip.clear();
    }
  }

  // Read full chips.
  while (samples.end() - sampleItr >= cfg->chipSamples) {
    assert(partialChip.empty());
    makeChip(samples, sampleItr);
    sampleItr += cfg->chipSamples;
  }

  // Store any remainder in partialChip
  if (sampleItr != samples.end()) {
    assert(partialChip.empty());
    assert(samples.end() - sampleItr < cfg->chipSamples);
    partialChip.insert(partialChip.end(), sampleItr, samples.end());
  }
  cerr << "Holding " << chips.size() << " chips + " << partialChip.size() << " samples" 
      << " after " << samples.size() << " more samples" << endl;
  assert(partialChip.size() < cfg->chipSamples);
}

void Receiver::makeChip(vector<float> &samples, vector<float>::iterator nextSample) {
  assert(nextSample >= samples.begin());
  float *firstSample = samples.data() + (nextSample - samples.begin());
  chips.push_back(new Spectrum(firstSample, SAMPLE_RATE, cfg->chipSamples));
}

void Receiver::takeChips(int numChips, vector<vector<float> > &target) {
  target.reserve(target.size() + numChips);
  deque<Spectrum*>::iterator last = chips.begin() + numChips;
  for (deque<Spectrum*>::iterator itr = chips.begin(); itr != last; ++itr) {
    target.push_back(vector<float>());
    vector<float>& chip = target.back();
    for (int i = cfg->baseBucket; i < cfg->baseBucket + (cfg->numChannels * cfg->channelSpacing); i += cfg->channelSpacing) {
      chip.push_back((*itr)->amplitude(i));
    }
    delete *itr;
  }
  chips.erase(chips.begin(), chips.begin() + numChips);
}
