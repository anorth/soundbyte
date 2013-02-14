#include "codecs.h"
#include "util.h"

#include <cassert>
#include <iostream>

void *init_rs_int(int symsize,int gfpoly,int fcr, int prim,int nroots,int pad);
void free_rs_int(void *rs);
void encode_rs_int(void *rs,int *data,int *parity);
int decode_rs_int(void *rs,int *data,int *eras_pos,int no_eras);

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
  ParamInputs inputs = defaultInputs[symsize];

  nroots = n - k;
  assert(nroots > 0);
  assert(n > nroots);
  int pad = (1 << symsize) - n - 1;

  params = init_rs_int(
      symsize, inputs.gfpoly, inputs.fcr, inputs.prim, nroots, pad);

  assert(params);
}

RsCodec::~RsCodec() {
  free_rs_int(params);
}

void RsCodec::encode(vector<char> &message, vector<bool> &target) {
  // TODO: symsize other than 8.
  assert(symsize == 8);
  assert(message.size() % blockSize == 0);

  for (int block = 0; block < message.size(); block += blockSize) {
    int parity[nroots];
    int data[blockSize];
    for (int i = 0; i < blockSize; i++) {
      data[i] = (unsigned char) message[block + i];
    }

    encode_rs_int(params, data, parity);

    vector<char> encoded;
    for (int i = 0; i < blockSize; i++) {
      encoded.push_back((char) data[i]);
    }
    for (int i = 0; i < nroots; i++) {
      encoded.push_back((char) parity[i]);
    }
    toBitSequence(encoded, target);
  }
}

int RsCodec::decode(vector<float> &bits, vector<char> &target) {
  toByteSequence(bits, target);
  return 0;
}

int RsCodec::numEncodedBitsForBytes(int nbytes) {
  return nbytes * 8;
}

int RsCodec::blockBytes() {
  return 1;
}


///// Utils

