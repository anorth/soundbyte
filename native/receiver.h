#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include <deque>
#include <queue>
#include <vector>

class Packeter;
class Sync;

class Receiver {
public:
  Receiver(Sync *sync, Packeter *packeter);

  void receiveAudio(std::vector<float> &samples);

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  int state;
  std::queue<std::vector<char> > messages;

  Sync *sync;
  Packeter *packeter;
};

#endif /* _RECEIVER_H_ */
