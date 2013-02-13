#include "packeter.h"

#include "util.h"

#include <iostream>

using namespace std;

Packeter::Packeter(Config *cfg, Codec *codec, Assigner *assigner) :
    cfg(cfg),
    codec(codec),
    assigner(assigner) {
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
  vector<bool> encodedBits;
  codec->encode(message, encodedBits);
  cerr << "Encoded " << message.size() << " bytes into " << encodedBits.size() << " bits" << endl;
  assigner->chipify(encodedBits, target);
  cerr << "Cooked " << encodedBits.size() << " bits into " << target.size() << " chips" << endl;
  //for (int i = 0; i < target.size(); ++i) {
  //  cerr << i << ": " << target[i] << endl;
  //}
}

int Packeter::decodeMessage(vector<vector<float> > &chips, vector<char> &target) {
  vector<float> encodedBits;
  //for (int i = 0; i < chips.size(); ++i) {
  //  cerr << "<-" << i << ": " << chips[i] << endl;
  //}
  assigner->unchipify(chips, encodedBits);
  cerr << "Recovered " << encodedBits.size() << " bits from " << chips.size() << " chips" << endl;
  int ret =  codec->decode(encodedBits, target);
  cerr << "Decoded " << target.size() << " bytes from " << encodedBits.size() << " bits" << endl;
  return ret;
}

int Packeter::numSymbolsForBytes(int nbytes) {
  return assigner->numSymbolsForBits(codec->numEncodedBitsForBytes(nbytes));
}
