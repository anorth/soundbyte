#ifndef DECODER_H_
#define DECODER_H_

#include <deque>
#include <queue>
#include <vector>

class Packeter;
class Sync;

class Decoder {
public:
  Decoder(Sync *sync, Packeter *packeter);

  void receiveAudio(std::vector<float> &samples);

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  int state;
  std::queue<std::vector<char> > messages;

  Sync *sync;
  Packeter *packeter;
};

#endif /* DECODER_H_ */
