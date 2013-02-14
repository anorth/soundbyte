#include "sender.h"

#include <cassert>
#include <iostream>

#include "audio.h"
#include "constants.h"
#include "packeter.h"
#include "sync.h"

using namespace std;

Sender::Sender(Config *cfg, Sync *sync, Packeter *packeter) :
  cfg(cfg),
  sync(sync),
  packeter(packeter) {
}

void Sender::encodeMessage(std::vector<char> &message, std::vector<float> &target) {
  // TODO(alex): variable-length packets
  assert(message.size() == TEST_MESSAGE_SIZE);

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

  cerr << "Generated " << nsamples << " samples for " << chips.size() << " chips" << endl;
}
