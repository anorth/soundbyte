#ifndef _UTIL_H_
#define _UTIL_H_

#include <vector>

// Counts bits set
int countBits(int);

// +/- 1.0
float signum(float);

// Partitions a vector in to vectors of length n
void partition(std::vector<float> sequence, int n, std::vector<std::vector<float> > target);

// Computes decibel ratio between two power quantities
float dbPower(float a, float b);

// Computes decibel ratio between two amplitude quantities
float dbAmplitude(float a, float b);

// Computes the power multiplier for a dB ratio
float dbPowerGain(float d);

// Computes the amplitude multiplier for a dB ratio
float dbAmplitudeGain(float d);

// Binomial coefficient, Cnk.
int comb(int n, int k);

// The k-combination corresponding to integer i, as a sequence of k combinatorial
// elements [ck, ck-1, ... c1]. This is a unique sequence such that
//   i = C(ck, k) + C(ck-1, k-1) + ... + C(c1, 1)
// This is logically equivalent to the i'th n-bit number with k bits set,
// with the combinatorial elements being the indexes of the bits set.
void combinadic(int k, int i, std::vector<int> &target);

// The integer corresponding to a k-combination as produced by combinadic()
int inverseCombinadic(int k, std::vector<int> elts);

// Computes the maximum Cnk <= m
// Returns (n, Cnk)
int maxCombElement(int k, int m, int *cnk);

#endif
