#include "util.h"

#include <cassert>
#include <cmath>

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
  assert(false);
  return 0;
}

void combinadic(int k, int i, std::vector<int> &target) {
}

int inverseCombinadic(int k, std::vector<int> elts) {
  assert(false);
  return 0;
}

int maxCombElement(int k, int m, int *cnk) {
  assert(false);
  return 0;
}

