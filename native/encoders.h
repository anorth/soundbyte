#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#include "encoder.h"

class IdentityEncoder : Encoder {
public:
  IdentityEncoder() {};
  void encode(std::vector<char> &message, std::vector<bool> &target);
  int decode(std::vector<float> &bits, std::vector<char> &target);
};

#endif
