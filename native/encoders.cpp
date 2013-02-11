#include "encoders.h"

#include "util.h"

using namespace std;


///// Identity encoder

void IdentityEncoder::encode(vector<char> &message, vector<bool> &target) {
  toBitSequence(message, target);
}

int IdentityEncoder::decode(vector<float> &bits, vector<char> &target) {
  toByteSequence(bits, target);
  return 0;
}

///// Utils

