#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#include "codec.h"

class IdentityCodec : public Codec {
public:
  IdentityCodec(int blockSize);
  virtual void encode(const std::vector<bool> &message, std::vector<bool> &target) const;
  virtual int decode(const std::vector<float> &bits, std::vector<bool> &target) const;
  virtual int blockMessageBits() const;
  virtual int blockEncodedBits() const;

private:
  int blockSize;
};

typedef void *RsParams;

class RsCodec : public Codec {
public:
  RsCodec(int encodedSize, int messageSize, int symSizeBits);
  virtual ~RsCodec();
  virtual void encode(const std::vector<bool> &message, std::vector<bool> &target) const;
  virtual int decode(const std::vector<float> &bits, std::vector<bool> &target) const;
  virtual int blockMessageBits() const;
  virtual int blockEncodedBits() const;

private:
  RsParams params;
  int nroots;
  int pad;
  int blockSize;
  int symsize;
};

#endif
