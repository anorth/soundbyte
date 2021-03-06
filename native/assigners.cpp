#include "assigners.h"

#include "util.h" 

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "util.h"

using namespace std;

///// CombinadicAssigner

CombinadicAssigner::CombinadicAssigner(int nchans) : 
    nchans(nchans),
    width(log(comb(nchans, nchans / 2)) / log(2)) {
  //cerr << "Assigner has nchans " << nchans << " width " << width << endl;
}

void CombinadicAssigner::chipify(vector<bool> &bits, vector<vector<bool> > &chips) {
  //cerr << "bits: " << bits << endl;
  int k = nchans / 2;
  vector<bool>::const_iterator it = bits.begin();
  while (it != bits.end()) {
    int i = nextInt(bits, it, width);
    vector<int> signalIndexes;
    combinadic(k, i, signalIndexes);
    //cerr << "i: " << i << " -> signal indexes: " << signalIndexes << endl;
    vector<bool> currentChip(nchans); // Zeros
    for (vector<int>::iterator si = signalIndexes.begin(); si != signalIndexes.end(); ++si) {
      assert(*si < currentChip.size());
      currentChip[*si] = 1.0;
    }
    // print currentChip
    assert(accumulate(currentChip.begin(), currentChip.end(), 0.0) == (float)k);
    chips.push_back(currentChip);
  }
}

void CombinadicAssigner::unchipify(vector<vector<float> > &chips, vector<float> &bits) {
  int k = nchans / 2;
  vector<vector<float> >::iterator chipItr;
  for (chipItr = chips.begin(); chipItr != chips.end(); ++chipItr) {
    // Partition signals into high and low half
    vector<float> &chip = *chipItr;
    assert(chip.size() == nchans);
    vector<pair<float, int> > indexedSignals;
    for (int i = 0; i < chip.size(); ++i) {
      indexedSignals.push_back(make_pair(chip[i], i));
    }

    sort(indexedSignals.rbegin(), indexedSignals.rend()); // Descending
    vector<pair<float, int> >::iterator mid = indexedSignals.begin() + (indexedSignals.size() / 2);
    vector<int> highIndexes;
    for (vector<pair<float, int> >::iterator it = indexedSignals.begin(); it != mid; ++it) {
      highIndexes.push_back(it->second);
    }
    // TODO(alex): calculate SNRs here, ratio of lowest on channel to highest
    // off channel

    sort(highIndexes.rbegin(), highIndexes.rend()); // Descending
    // print indexes
    assert(highIndexes.size() == k);
    int n = inverseCombinadic(k, highIndexes);
    //cerr << "high indexes " << highIndexes << " -> " << n << endl;
    vector<float> chipBits;
    toFloatBits(n, width, chipBits);
    bits.insert(bits.end(), chipBits.begin(), chipBits.end());
  }
}

int CombinadicAssigner::numSymbolsForBits(int nbits) {
  return int(ceil(1.0 * nbits / width));
}

int CombinadicAssigner::bitsPerSymbol() {
  return width;
}
