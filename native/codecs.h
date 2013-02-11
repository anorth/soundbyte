#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#include "codec.h"

class IdentityCodec : Codec {
public:
  IdentityCodec() {};
  void encode(std::vector<char> &message, std::vector<bool> &target);
  int decode(std::vector<float> &bits, std::vector<char> &target);
};

#endif
