#include "packeter.h"

using namespace std;

Packeter::Packeter(Encoder &encoder, Assigner &assigner) :
    encoder(&encoder),
    assigner(&assigner) {
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
  vector<bool> encodedBits;
  encoder->encode(message, encodedBits);
  assigner->chipify(encodedBits, target);
}

int Packeter::decodeMessage(vector<vector<float> > &chips, vector<char> &target) {
  vector<float> encodedBits;
  assigner->unchipify(chips, encodedBits);
  return encoder->decode(encodedBits, target);
}

