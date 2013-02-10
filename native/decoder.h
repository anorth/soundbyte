#ifndef DECODER_H_
#define DECODER_H_

#include <deque>
#include <queue>
#include <vector>

class Decoder {
public:
  Decoder();

  void receiveAudio(std::vector<float> &samples);

  bool messageAvailable();

  int takeMessage(char *buffer, int bufferCapacity);

private:
  std::deque<float> buffer;
  std::queue<float> messages;
};

#endif /* DECODER_H_ */
