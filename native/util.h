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

void toBitSequence(std::vector<char> &message, std::vector<bool> &target);

void toByteSequence(std::vector<float> &bits, std::vector<char> &target);

// Reads 8 bit-likelihoods from it and returns a byte
char nextByte(std::vector<float> &bits, std::vector<float>::iterator &it);

// Reads nbits bits from it and returns an int
int nextInt(std::vector<bool> &bits, std::vector<bool>::iterator &it, int nbits);

// Writes nbits bit likelihoods from integer to target
void toBits(int integer, int nbits, std::vector<float> &target);


#endif