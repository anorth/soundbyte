#include "packeter.h"

#include "util.h"

#include <iostream>
#include <cassert>

using namespace std;

Packeter::Packeter(Config *cfg, Codec *codec, Assigner *assigner) :
    cfg(cfg),
    codec(codec),
    assigner(assigner) {
  remaining = 0;
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
  vector<bool> encodedBits;

  assert(message.size() <= 255);
  vector<char> packet;
  packet.push_back((unsigned char) message.size());
  packet.insert(packet.end(), message.begin(), message.end());
  assert(packet.size() == message.size() + 1);

  int size = packet.size(), block = codec->blockBytes();
  if (size % block > 0) {
    packet.resize(size + block - (size % block));
  }
  assert(packet.size() % block == 0);

  codec->encode(packet, encodedBits);
  cerr << "Encoded " << packet.size() << " bytes into " << encodedBits.size() << " bits" << endl;
  cerr << encodedBits << endl;
  assigner->chipify(encodedBits, target);
  cerr << "Cooked " << encodedBits.size() << " bits into " << target.size() << " chips" << endl;
  for (int i = 0; i < target.size(); ++i) {
    cerr << i << ": " << target[i] << endl;
  }
}

int Packeter::decodePartial(vector<vector<float> > &chips, vector<char> &target) {
  assert(chips.size() == chunkChips());

  vector<float> encodedBits;
  cerr << "Decoding " << chips.size() << " chips" << endl;
  for (int i = 0; i < chips.size(); ++i) {
    cerr << i << ": " << chips[i] << endl;
  }
  assigner->unchipify(chips, encodedBits);
  cerr << "Recovered " << encodedBits.size() 
       << " bits from " << chips.size() << " chips" << endl;
  cerr << encodedBits << endl;

  vector<char> decoded;
  int error =  codec->decode(encodedBits, target);
  if (error) {
    return -1;
  }
  cerr << "Decoded " << target.size() << " bytes from " 
       << encodedBits.size() << " bits" << endl;

  vector<char>::iterator it = decoded.begin();
  if (remaining == 0) {
    remaining = (unsigned char) (*it);
    it++;
  }

  while (it != decoded.end() && remaining > 0) {
    target.push_back(*it);

    it++;
    remaining--;
  }

  return remaining;
}

int Packeter::chunkChips() {
  return assigner->numSymbolsForBits(codec->blockBytes());
}
