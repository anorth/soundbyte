#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include <deque>
#include <queue>
#include <vector>

#include "config.h"
#include "spectrum.h"
#include "stream.h"

class Packeter;
class Sync;

// maybe make this a stream too
class Receiver {
public:
  Receiver(Config *cfg, Stream<float>& source, Sync *sync, Packeter *packeter);

  /* returns an integer vaguely indicative of progress (useful for UI) */
  int receiveAudio();

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  int state; // actual state
  int progress; // progress indicator based on state and other factors
  int subProgress;
  std::vector<char> partialMessage;
  std::queue<std::vector<char> > messages;

  Stream<float>& source;
  Config *cfg;
  Sync *sync;
  Packeter *packeter;

  void receiveChips(int numChips, std::vector<std::vector<float> > &target);
};

#endif /* _RECEIVER_H_ */
