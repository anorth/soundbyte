#ifndef _SENDER_H_
#define _SENDER_H_

#include <vector>

#include "config.h"

class Packeter;
class Sync;

class Sender {
public:
  Sender(Config *cfg, Sync *sync, Packeter *packeter);

  /** Encodes a complete message into a waveform. */
  void encodeMessage(std::vector<char> &message, std::vector<float> &target);

private:
  Config *cfg;
  Sync *sync;
  Packeter *packeter;
};

#endif

