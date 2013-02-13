#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include <deque>
#include <queue>
#include <vector>

#include "config.h"
#include "spectrum.h"

class Packeter;
class Sync;

class Receiver {
public:
  Receiver(Config *cfg, Sync *sync, Packeter *packeter);

  void receiveAudio(std::vector<float> &samples);

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  int state;
  std::deque<Spectrum*> chips; // Queue of spectrums
  std::vector<float> partialChip; // Samples from an incomplete chip
  std::queue<std::vector<char> > messages;

  Config *cfg;
  Sync *sync;
  Packeter *packeter;

  void receiveChips(std::vector<float> &samples);
  void makeChip(std::vector<float> &samples, std::vector<float>::iterator nextSample);
  void takeChips(int numChips, std::vector<std::vector<float> > &target);
};

#endif /* _RECEIVER_H_ */
