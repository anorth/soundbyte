#ifndef _CODEC_H_
#define _CODEC_H_

#include <vector>

class Codec {
public:
  /** Encodes raw data bits into encoded bits. */
  virtual void encode(const std::vector<bool> &message, std::vector<bool> &target) const = 0;

  /** Decodes bit likelihoods into raw data bits. Returns 0 or positive on success, negative error. */
  virtual int decode(const std::vector<float> &bits, std::vector<bool> &target) const = 0;

  /** Returns the size in bits which the input to encode() must be a multiple. */
  virtual int blockMessageBits() const = 0;

  /** Returns the size in bits which the input to decode() must be a multiple. */
  virtual int blockEncodedBits() const = 0;

protected:
  Codec() {};
  virtual ~Codec() {};
};

#endif

