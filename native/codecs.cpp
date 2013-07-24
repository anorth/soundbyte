#include "codecs.h"
#include "util.h"
#include "log.h"

#include "int.h"  // rs

#include <cassert>
#include <iostream>

//void *init_rs_int(int symsize,int gfpoly,int fcr, int prim,int nroots,int pad);
//void free_rs_int(void *rs);
//void encode_rs_int(void *rs,int *data,int *parity);
//int decode_rs_int(void *rs,int *data,int *eras_pos,int no_eras);

using namespace std;

static const char* TAG = "SoundbyteCodec";

///// Identity codec

IdentityCodec::IdentityCodec(int blockSize) : blockSize(blockSize) {}

void IdentityCodec::encode(const vector<bool> &message, vector<bool> &target) const {
  target.insert(target.end(), message.begin(), message.end());
}

int IdentityCodec::decode(const vector<float> &bits, vector<bool> &target) const {
  assert(0); // TODO: implement
  // like this, but with right type target.insert(target.end(), bits.begin(), bits.end());
  return 0;
}

int IdentityCodec::blockMessageBits() const {
  return blockSize * 8;
}

int IdentityCodec::blockEncodedBits() const {
  return blockSize * 8;
}


//// Reed Solomon codec

/*
 * Default parameters by symbol size
 * Taken from python module c binding
 */
typedef struct {
    int gfpoly;
    int fcr;
    int prim;
} ParamInputs;

ParamInputs defaultInputs[] = {
    /*  0 */ {     -1,  -1, -1},
    /*  1 */ {     -1,  -1, -1},
    /*  2 */ {    0x7,   1,  1},
    /*  3 */ {    0xb,   1,  1},
    /*  4 */ {   0x13,   1,  1},
    /*  5 */ {   0x25,   1,  1},
    /*  6 */ {   0x43,   1,  1},
    /*  7 */ {   0x89,   1,  1},
    /*  8 */ {  0x187, 112, 11}, /* Based on CCSDS codec */
    /*  9 */ {  0x211,   1,  1},
    /* 10 */ {  0x409,   1,  1},
    /* 11 */ {  0x805,   1,  1},
    /* 12 */ { 0x1053,   1,  1},
    /* 13 */ { 0x201b,   1,  1},
    /* 14 */ { 0x4443,   1,  1},
    /* 15 */ { 0x8003,   1,  1},
    /* 16 */ {0x1100b,   1,  1},
};

RsCodec::RsCodec(int n, int k, int symsize) : blockSize(k), symsize(symsize) {
  assert(symsize >= 2 && symsize <= 16);
  assert(n < (1<<symsize) );
  ParamInputs inputs = defaultInputs[symsize];

  nroots = n - k;
  assert(nroots > 0);
  assert(n > nroots);
  pad = (1 << symsize) - n - 1;

  params = init_rs_int(
      symsize, inputs.gfpoly, inputs.fcr, inputs.prim, nroots, pad);

  assert(params);
}

RsCodec::~RsCodec() {
  free_rs_int(params);
}

void RsCodec::encode(const vector<bool> &message, vector<bool> &target) const {
  // TODO: symsize other than 8.
  assert(symsize > 1);
  //assert(message.size() % blockSize == 0);

  int blockBits = blockMessageBits();

  vector<bool>::const_iterator it = message.begin();
  for (int block = 0; block < message.size(); block += blockBits) {
    int parity[nroots];
    int data[blockSize];
    for (int i = 0; i < blockSize; i++) {
      //data[i] = unpackBits(&message[block], i * symsize, chunkBits);
      // TODO: use proper iterator
      data[i] = nextInt(message, it, symsize);
    }

    encode_rs_int(params, data, parity);

    for (int i = 0; i < blockSize; i++) {
      toBits(symsize, data[i], target);
    }
    for (int i = 0; i < nroots; i++) {
      toBits(symsize, parity[i], target);
    }

    // test errors
    // encoded[1] = '.';
    // encoded[3] = '.';
    // encoded[4] = '.';
    // encoded[5] = '.';
    // encoded[6] = '.';
  }
}

int RsCodec::decode(const vector<float> &floatBits, vector<bool> &target) const {
  assert(symsize > 1);
  assert(floatBits.size() == blockEncodedBits());

  vector<bool> bits;
  for (vector<float>::const_iterator it = floatBits.begin(); it != floatBits.end(); it++) {
    bits.push_back( (*it) > 0.0f );
  }

  vector<int> rawSymbols;
  toSymbolSequence(symsize, bits, rawSymbols);

  int errors = 0;
  int erasures[nroots];
  int count = decode_rs_int(params, rawSymbols.data(), erasures, 0);

  if (count < 0) {
    ll(LOG_INFO, TAG, "Too many errors (%d)", count);
    return -1;
  }

  for (int i = 0; i < count; i++) {
    int index = erasures[i] - pad;
    if (index < 0) {
      ll(LOG_INFO, TAG, "Corrupted input");
      return -1;
    }
  }

  ll(LOG_DEBUG, TAG, "Block OK");

  errors += count;

  for (int i = 0; i < blockSize; i++) {
    toBits(symsize, rawSymbols.data()[i], target);
  }

  return 0;
}

int RsCodec::blockMessageBits() const {
  return blockSize * symsize;
}

int RsCodec::blockEncodedBits() const {
  return (blockSize + nroots) * symsize;
}

///// Utils

