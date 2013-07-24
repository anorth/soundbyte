#include "packeter.h"
#include "log.h"
#include "util.h"

#include <iostream>
#include <cassert>

using namespace std;

// TODO: configuration constant
#define SIZE_BITS 8

static const char* TAG = "SoundbytePacketer";

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
  ll(LOG_INFO, TAG, "Encoded %ld bits into %ld bits", packetBits.size(), encodedBits.size());
//  cerr << encodedBits << endl;

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
    ll(LOG_DEBUG, TAG, "Cooked %ld bits into %ld chips", tmp.size(), target.size());
  }

//  for (int i = 0; i < target.size(); ++i) {
//    cerr << i << ": " << target[i] << endl;
//  }
}

int Packeter::decodePartial(vector<vector<float> > &chips, vector<bool> &target) {
  assert(chips.size() == chunkChips());

  vector<float> encodedBits;
  ll(LOG_DEBUG, TAG, "Decoding %ld chips", chips.size());
//  for (int i = 0; i < chips.size(); ++i) {
//    cerr << i << ": " << chips[i] << endl;
//  }

  assigner->unchipify(chips, encodedBits);
  assert(encodedBits.size() >= codec->blockEncodedBits());
  assert(encodedBits.size() < codec->blockEncodedBits() * 2); // This upper bound can be tightened but i cbb
  encodedBits.resize(codec->blockEncodedBits());
  ll(LOG_DEBUG, TAG, "Recovered %ld bits from %ld chips", encodedBits.size(), chips.size());
//  cerr << encodedBits << endl;

  vector<bool> decoded;
  int error =  codec->decode(encodedBits, decoded);
  if (error) {
    ll(LOG_INFO, TAG, "Packet Dropped\n");
    return -1;
  }
  ll(LOG_INFO, TAG, "Chunk received\n");
  ll(LOG_DEBUG, TAG, "Decoded %ld bits from %ld bits", decoded.size(), encodedBits.size());
  vector<bool>::const_iterator it = decoded.begin();
  if (remainingBits == 0) {
    remainingBits = nextInt(decoded, it, SIZE_BITS) << 3;
    ll(LOG_DEBUG, TAG, "First chunk, %d bits remaining =============", remainingBits);

  } else {
    ll(LOG_DEBUG, TAG, "Subsequent chunk, %d bits remaining", remainingBits);
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
