#include "packeter.h"
#include "log.h"
#include "util.h"

#include <iostream>
#include <cassert>

using namespace std;

// TODO: configuration constant
#define SIZE_BITS 8

Packeter::Packeter(Config *cfg, Codec *codec, Assigner *assigner) :
    cfg(cfg),
    codec(codec),
    assigner(assigner) {
  remainingBits = 0;
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
  vector<bool> encodedBits;

  assert(message.size() <= 255);

  vector<bool> packetBits;
  assert(message.size() < (1 << SIZE_BITS));
  toBits(SIZE_BITS, message.size(), packetBits);
  assert(packetBits.size() == SIZE_BITS);

  // todo: fix
  vector<bool> tmp;
  toBitSequence(message, tmp);
  packetBits.insert(packetBits.end(), tmp.begin(), tmp.end());
  assert(packetBits.size() == message.size() * 8 + SIZE_BITS);

  int size = packetBits.size(), block = codec->blockMessageBits();
  if (size % block > 0) {
    packetBits.resize(size + block - (size % block));
  }
  assert(packetBits.size() % block == 0);

  codec->encode(packetBits, encodedBits);
  cerr << "Encoded " << packetBits.size() << " bites into " << encodedBits.size() << " bits" << endl;
  cerr << encodedBits << endl;

  // TODO: use iterator ranges or whatever
  int bitBlock = codec->blockEncodedBits();
  for (vector<bool>::iterator it = encodedBits.begin();
      it != encodedBits.end();
      it += bitBlock) {
    assert(it < encodedBits.end());
    vector<bool> tmp;

    vector<bool>::iterator it2 = it;

    tmp.insert(tmp.end(), it2, it + bitBlock);

    assigner->chipify(tmp, target);
    cerr << "Cooked " << tmp.size() << " bits into " << target.size() << " chips" << endl;
  }

  for (int i = 0; i < target.size(); ++i) {
    cerr << i << ": " << target[i] << endl;
  }
}

int Packeter::decodePartial(vector<vector<float> > &chips, vector<bool> &target) {
  assert(chips.size() == chunkChips());

  vector<float> encodedBits;
  cerr << "Decoding " << chips.size() << " chips" << endl;
  for (int i = 0; i < chips.size(); ++i) {
    cerr << i << ": " << chips[i] << endl;
  }
  assigner->unchipify(chips, encodedBits);
  assert(encodedBits.size() >= codec->blockEncodedBits());
  assert(encodedBits.size() < codec->blockEncodedBits() * 2); // This upper bound can be tightened but i cbb
  encodedBits.resize(codec->blockEncodedBits());
  cerr << "Recovered " << encodedBits.size() 
       << " bits from " << chips.size() << " chips" << endl;
  cerr << encodedBits << endl;

  vector<bool> decoded;
  int error =  codec->decode(encodedBits, decoded);
  if (error) {
    ll(LOG_INFO, "SCOM", "Packet Dropped\n");
    assert(false);
    return -1;
  }
  ll(LOG_INFO, "SCOM", "Chunk received\n");
  cerr << "Decoded " << decoded.size() << " bits from " 
       << encodedBits.size() << " bits" << endl;

  vector<bool>::const_iterator it = decoded.begin();
  if (remainingBits == 0) {
    remainingBits = nextInt(decoded, it, SIZE_BITS) << 3;
    cerr << "First chunk, bits remaining =========================== " << remainingBits << endl;
  } else {
    cerr << "Subsequent chunk, bits remaining " << remainingBits << endl;
  }

  while (it != decoded.end() && remainingBits > 0) {
    target.push_back(*it);

    it++;
    remainingBits--;
  }
  //vector<char> foo;
  //toByteSequence(target, foo);
  //cout << " SO FAR ----------------- " << foo << endl;

  return remainingBits;
}

int Packeter::chunkChips() {
  return assigner->numSymbolsForBits(codec->blockEncodedBits());
}
