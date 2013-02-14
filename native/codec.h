#ifndef _CODEC_H_
#define _CODEC_H_

#include <vector>

class Codec {
public:
  /** Encodes raw data into encoded bits. */
  virtual void encode(std::vector<char> &message, std::vector<bool> &target) = 0;

  /** Decodes bit likelihoods into raw data. Returns 0 or positive on success, negative error. */
  virtual int decode(std::vector<float> &bits, std::vector<char> &target) = 0;

  /** Returns the size in bytes which the input to encode() must be a multiple. 
      Can of course return 1. */
  virtual int blockMessageBytes() = 0;

  /** Returns the size in bytes which the input to decode() must be a multiple. */
  virtual int blockEncodedBytes() = 0;

protected:
  Codec() {};
};

#endif

