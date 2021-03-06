#include "util.h"

#include <cassert>
#include <cmath>
#include <vector>

using namespace std;

int countBits(int v) {
  int c = 0;
  while (v != 0) {
    v &= v - 1; // clear the least significant bit set
    c += 1;
  }
  return c;
}

float signum(float x) {
  assert(x != 0); // You're doing it wrong
  return x > 0.0 ? 1.0 : -1.0;
}

void partition(std::vector<float> sequence, int n, std::vector<std::vector<float> > target) {
  // To be implemented.
  assert(false);
}

float dbPower(float a, float b) {
  return 10.0 * log10(a / b);
}

float dbAmplitude(float a, float b) {
  return 2.0 * dbPower(a, b);
}

float dbPowerGain(float d) {
  return pow(10.0, d / 10.0);
}

float dbAmplitudeGain(float d) {
  return sqrt(dbPowerGain(d));
}

int comb(int n, int k) {
  // http://rosettacode.org/wiki/Evaluate_binomial_coefficients#C
  // Handling for k > n added by Alex
	int r = 1, d = n - k;
 
  if (k > n) { return 0; }
	/* choose the smaller of k and n - k */
	if (d > k) { k = d; d = n - k; }
 
	while (n > k) {
		r *= n--;
 
		/* divide (n - k)! as soon as we can to delay overflows */
		while (d > 1 && !(r % d)) {
      r /= d--;
    }
	}
	return r;
}

void combinadic(int k, int i, std::vector<int> &target) {
  target.clear();
  int kk = k;
  int n = i;
  while (n > 0) {
    int cnk;
    int el = maxCombElement(kk, n, &cnk);
    target.push_back(el);
    n -=  cnk;
    kk -= 1;
  }
  for (int j = target.size(); j < k; ++j) {
    target.push_back(k - j - 1);
  }
}

int inverseCombinadic(int k, std::vector<int> elts) {
  int i = 0;
  for (vector<int>::iterator it = elts.begin(); it != elts.end(); ++it) {
    i += comb(*it, k);
    k -= 1;
  }
  return i;
}

int maxCombElement(int k, int m, int *cnk) {
  int cc = 0;
  int n = k - 1;
  int c = comb(n, k);
  while (c <= m) {
    cc = c;
    n += 1;
    c = comb(n, k);
  }
  *cnk = cc;
  return n - 1;
}

int toBitSequence(const vector<char> &message, vector<bool> &target) {
  target.reserve(target.size() + message.size() * 8);
  for (vector<char>::const_iterator it = message.begin(); it != message.end(); ++it) {
    char b = *it;
    for (int i = 0; i < 8; ++i) {
      target.push_back((b & (1 << i)) >> i);
    }
  }
  return message.size() * 8;
}

int toBitSequence(const vector<char> &message, vector<bool> &target, int symbolWidth) {
  int bits = toBitSequence(message, target);
  int overflow = bits % symbolWidth;
  if (overflow) {
    // Pad with zero
    target.insert(target.end(), symbolWidth - overflow, false);
  }
  return bits + (symbolWidth - overflow);
}

// Reads n bit-likelihoods from it and returns a symbol
int nextInt(const vector<bool> &bits, vector<bool>::const_iterator &it, int nbits) {
  int v = 0;
  for (int i = 0; i < nbits && it != bits.end(); ++i, ++it) {
    if (*it) { 
      v |= 1 << i;
    }
  }
  return v;
}


char nextByte(const vector<bool> &bits, vector<bool>::const_iterator &it) {
  char v = 0;
  for (int i = 0; i < 8 && it != bits.end(); ++i, ++it) {
    if (*it) { 
      v |= 1 << i;
    }
  }
  return v;
}


int toSymbolSequence(int symbolBits, const vector<bool> &bits, vector<int> &target) {
  int overflow = bits.size() % symbolBits;
  vector<bool>::const_iterator end = bits.end() - overflow;
  vector<bool>::const_iterator it = bits.begin();
  while (it != end) {
    target.push_back(nextInt(bits, it, symbolBits));
  }
}

int toByteSequence(const vector<bool> &bits, vector<char> &target) {
  int overflow = bits.size() % 8;
  vector<bool>::const_iterator end = bits.end() - overflow;
  vector<bool>::const_iterator it = bits.begin();
  while (it != end) {
    target.push_back(nextByte(bits, it));
  }
}


void toBits(unsigned char numbits, int val, vector<bool> &target) {
  assert(numbits < 32);

  for (int i = 0; i < numbits; ++i) {
    target.push_back((val & (1 << i)) >> i);
  }
}



void toFloatBits(int integer, int nbits, vector<float> &target) {
  for (int i = 0; i < nbits; ++i) {
    if ((integer & (1 << i)) >> i) {
      target.push_back(1.0f);
    } else {
      target.push_back(-1.0f);
    }
  }
}

unsigned int unpackBits(unsigned char *ptr, int offset, int nbits) {
  assert(sizeof(unsigned char) == 1);

  int index = offset >> 3;
  int bitOffset = offset & 7;
  int result = 0;
  int shift = 0;
  //cout << "START " << endl;
  while (nbits > 0) {
    // TODO: word at a time, not byte at a time
    int maxChunk = 8 - bitOffset;
    int chunkBits = (nbits > maxChunk ? maxChunk : nbits) ;
    int mask = (1<<chunkBits) - 1;

    int chunk = (ptr[index] >> bitOffset) & mask;
    //cout << "cb:" << chunkBits << " bo:" << bitOffset << " sh:" << shift << " ch:" << chunk;
    
    result |= chunk << shift;

    //cout << " r:" << result << endl;

    nbits -= chunkBits;
    shift += chunkBits;
    index++;
    bitOffset = 0;
  }

  return result;
}

void packBits(unsigned char *ptr, int offset, int nbits, unsigned int value) {
  assert(value < (1<<nbits));
  int index = offset >> 3;
  int bitOffset = offset & 7;
  int shift = 0;
  //cout << "START " << endl;
  while (nbits > 0) {
    int maxChunk = 8 - bitOffset;
    int chunkBits = (nbits > maxChunk ? maxChunk : nbits) ;
    int mask = (1<<chunkBits) - 1;

    int chunk = (value >> shift) & mask;
    //cout << "cb:" << chunkBits << " bo:" << bitOffset << " sh:" << shift << " ch:" << chunk
    //  << " mask:" << mask << " ~m:" << (~mask) << " cur:" << (int) ptr[index] << endl;
    ptr[index] = (ptr[index] & ~(mask << bitOffset)) | (chunk << bitOffset);

    //cout << "cb:" << chunkBits << " bo:" << bitOffset << " sh:" << shift << " ch:" << chunk;

    nbits -= chunkBits;
    shift += chunkBits;
    index++;
    bitOffset = 0;
  }
}
