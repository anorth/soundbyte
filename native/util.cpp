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
	int r = 1, d = n - k;
 
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
  int kk = k;
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
    target.push_back(nextByte(bits, it));
  }
}

char nextByte(vector<float> &bits, vector<float>::iterator &it) {
  char v = 0;
  for (char i = 0; i < 8 && it != bits.end(); ++i, ++it) {
    if (*it > 0.0f) { 
      v |= 1 << i;
    }
  }
  return v;
}

int nextInt(vector<bool> &bits, vector<bool>::iterator &it, int nbits) {
  int v = 0;
  for (int i = 0; i < nbits && it != bits.end(); ++i, ++it) {
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

