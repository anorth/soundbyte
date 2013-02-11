#include "encoders.h"

#include <cassert>

using namespace std;

void toBitSequence(vector<char> &message, vector<bool> &target);
void toByteSequence(vector<float> &bits, vector<char> &target);
// Reads 8 bit-likelihoods from it and returns a byte
char nextByte(vector<float>::iterator &it);
// Writes nbits bit likelihoods from integer to target
void toBits(int integer, int nbits, vector<float> &target);

///// Identity encoder

void IdentityEncoder::encode(vector<char> &message, vector<bool> &target) {
  toBitSequence(message, target);
}

int IdentityEncoder::decode(vector<float> &bits, vector<char> &target) {
  toByteSequence(bits, target);
  return 0;
}

///// Utils

void toBitSequence(vector<char> &message, vector<bool> &target) {
  target.clear();
  target.reserve(message.size() * 8);
  for (vector<char>::iterator it = message.begin(); it != message.end(); ++it) {
    char b = *it;
    for (int i = 0; i < 8; ++i) {
      target.push_back((b & (1 << i)) >> i);
    }
  }
}

void toByteSequence(vector<float> &bits, vector<char> &target) {
  assert(bits.size() % 8 == 0);
  vector<float>::iterator it = bits.begin();
  while (it != bits.end()) {
    target.push_back(nextByte(it));
  }
}

char nextByte(vector<float>::iterator &it) {
  char v = 0;
  for (char i = 0; i < 8; ++i, ++it) {
    if (*it > 0.0f) { 
      v |= 1 << i;
    }
  }
  return v;
}

void toBits(int integer, int nbits, vector<float> &target) {
  for (int i = 0; i < nbits; ++i) {
    if ((integer & (1 << i)) >> i) {
      target.push_back(1.0f);
    } else {
      target.push_back(-1.0f);
    }
  }
}
