#include "sender.h"

#include <cassert>

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
  // TODO(alex): generate sync preamble
  assert(message.size() == TEST_MESSAGE_SIZE);

  vector<vector<bool> > chips;
  packeter->encodeMessage(message, chips);
  for (vector<vector<bool> >::iterator chit = chips.begin(); chit != chips.end(); ++chit) {
    // TODO(alex): fade in/out each chip by chipSamples / 10
    buildWaveform(*chit, cfg->baseFrequency, cfg->channelSpacing, cfg->chipSamples, 
        target);
  }
}
