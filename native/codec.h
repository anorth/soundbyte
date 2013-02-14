#ifndef _CODEC_H_
#define _CODEC_H_

#include <vector>

class Codec {
public:
  /** Encodes raw data into encoded bits. */
  virtual void encode(std::vector<char> &message, std::vector<bool> &target) = 0;

  /** Decodes bit likelihoods into raw data. Returns 0 on success, negative error. */
  virtual int decode(std::vector<float> &bits, std::vector<char> &target) = 0;

  /** Returns the number of bits required to encode some bytes. */
  virtual int numEncodedBitsForBytes(int nbytes) = 0;

  /** Returns the size in bytes which the input to encode() must be a multiple. 
      The input to decode must be a multiple of numEncodedBitsForBytes(blockSize()).
      Can of course return 1. */
  virtual int blockSize() = 0;

protected:
  Codec() {};
};

#endif

