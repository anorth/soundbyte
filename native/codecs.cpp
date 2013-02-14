#include "codecs.h"

#include "util.h"

using namespace std;


///// Identity codec

IdentityCodec::IdentityCodec(int blockSize) : blockSize(blockSize) {}

void IdentityCodec::encode(vector<char> &message, vector<bool> &target) {
  toBitSequence(message, target);
}

int IdentityCodec::decode(vector<float> &bits, vector<char> &target) {
  toByteSequence(bits, target);
  return 0;
}

int IdentityCodec::numEncodedBitsForBytes(int nbytes) {
  return nbytes * 8;
}

int IdentityCodec::blockBytes() {
  return 1;
}

///// Utils

