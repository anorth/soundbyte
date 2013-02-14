#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#include "codec.h"

class IdentityCodec : public Codec {
public:
  IdentityCodec(int blockSize);
  void encode(std::vector<char> &message, std::vector<bool> &target);
  int decode(std::vector<float> &bits, std::vector<char> &target);
  int blockMessageBytes();
  int blockEncodedBytes();

private:
  int blockSize;
};

typedef void *RsParams;

class RsCodec : public Codec {
public:
  RsCodec(int encodedSize, int messageSize, int symSizeBits);
  ~RsCodec();
  void encode(std::vector<char> &message, std::vector<bool> &target);
  int decode(std::vector<float> &bits, std::vector<char> &target);
  int blockMessageBytes();
  int blockEncodedBytes();

private:
  RsParams params;
  int nroots;
  int pad;
  int blockSize;
  int symsize;
};

#endif
