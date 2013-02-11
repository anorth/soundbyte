#include "packeter.h"

using namespace std;

Packeter::Packeter(Codec *codec, Assigner *assigner) :
    codec(codec),
    assigner(assigner) {
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
  vector<bool> encodedBits;
  codec->encode(message, encodedBits);
  assigner->chipify(encodedBits, target);
}

int Packeter::decodeMessage(vector<vector<float> > &chips, vector<char> &target) {
  vector<float> encodedBits;
  assigner->unchipify(chips, encodedBits);
  return codec->decode(encodedBits, target);
}

