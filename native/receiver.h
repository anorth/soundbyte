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

  /* returns an integer vaguely indicative of progress (useful for UI) */
  int receiveAudio(std::vector<float> &samples);

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  int state; // actual state
  int progress; // progress indicator based on state and other factors
  int subProgress;
  std::vector<float> buffer;
  std::vector<char> partialMessage;
  std::queue<std::vector<char> > messages;

  Config *cfg;
  Sync *sync;
  Packeter *packeter;

  std::vector<float>::iterator receiveChips(std::vector<float> &samples, 
    std::vector<float>::iterator firstSample, int numChips, 
    std::vector<std::vector<float> > &target);
};

#endif /* _RECEIVER_H_ */
