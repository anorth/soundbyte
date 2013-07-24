#include "sender.h"

#include <cassert>
#include <iostream>

#include "audio.h"
#include "constants.h"
#include "packeter.h"
#include "sync.h"
#include "log.h"

using namespace std;

static const char* TAG = "SoundbyteSender";

Sender::Sender(Config *cfg, Sync *sync, Packeter *packeter) :
  cfg(cfg),
  sync(sync),
  packeter(packeter) {
}

void Sender::encodeMessage(std::vector<char> &message, std::vector<float> &target) {
  sync->generateSync(target);

  vector<vector<bool> > chips;
  packeter->encodeMessage(message, chips);
  int nsamples = 0;
  for (vector<vector<bool> >::iterator chit = chips.begin(); chit != chips.end(); ++chit) {
    // TODO(alex): fade in/out each chip by chipSamples / 10
    int signalSpacing = cfg->channelSpacing * cfg->channelWidth;
    buildWaveform(*chit, cfg->baseFrequency, signalSpacing, cfg->chipSamples, target);
    nsamples += cfg->chipSamples;
  }

  ll(LOG_DEBUG, TAG, "Generated %d samples for %ld chips", nsamples, chips.size());
}
