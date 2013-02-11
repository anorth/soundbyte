#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <vector>

class Encoder {
public:
  /** Encodes raw data into encoded bits. */
  virtual void encode(std::vector<char> &message, std::vector<bool> &target) = 0;

  /** Decodes bit likelihoods into raw data. Returns 0 on success, non-zero error. */
  virtual int decode(std::vector<float> &bits, std::vector<char> &target) = 0;

protected:
  Encoder() {};
};

#endif

