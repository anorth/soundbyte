#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#include "codec.h"

class IdentityCodec : public Codec {
public:
  IdentityCodec(int blockSize);
  void encode(std::vector<char> &message, std::vector<bool> &target);
  int decode(std::vector<float> &bits, std::vector<char> &target);
  int numEncodedBitsForBytes(int nbytes);
  int blockBytes();

private:
  int blockSize;
};

#endif
