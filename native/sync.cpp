#include "sync.h"

#include "config.h"
#include "constants.h"
#include "spectrum.h"

#include <iostream>
#include <cmath>
#include <cassert>

using namespace std;

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 2; // 2s

Sync::Sync(Config* cfg) {
  this->cfg = cfg;

  samplesPerMetaSample = (float) cfg->sync.syncChipSize / cfg->sync.detectionSamplesPerChip;
  pattern = new complex<float>[cfg->sync.numSyncChannels];

  resetSync();

  assert(cfg->sync.numSyncChannels % 2 == 0);
  bitPatternAbs = sqrt((float)cfg->sync.numSyncChannels/2);
}

Sync::~Sync() {
  delete pattern;
}

void Sync::generateSync(vector<float> &target) {
  
}

void Sync::resetSync() {
  fftSampleIndex = 0;
}

void Sync::copyBucketVals(Spectrum &spectrum, complex<float> *out) {
  for (int i = 0; i < cfg->sync.numSyncChannels; i++) {
    out[i] = spectrum.at(cfg->sync.syncBaseBucket + i * cfg->channelSpacing);
  }
}

float Sync::detectMatch(complex<float> *bucketVals) {
  // Computes dot-product of normalized bucketVals with normalized bit pattern.

  float bucketSum = 0.0;
  float result = 0;

  for (int i = 0; i < cfg->sync.numSyncChannels; i++) {
    float a = abs(bucketVals[i]);
    // Keep track of sum of squares, to normalise at end
    bucketSum += a*a;

    // Sync match bit pattern is 1,0,1,0,1,0...
    // Therefore add every 2nd one
    if (i % 2 == 0) {
      result += a;
    }
  }
  // Normalize
  result /= sqrt(bucketSum);
  result /= bitPatternAbs;

  // can rarely get NaNs, guard against
  result *= 0.99999;
  result += 0.000005;

  // Convert to linear correlation
  result = 1.57079632679f - 2*acos(result);

  // Map to range -1,1, with sinewave correlation
  result = sin(result);

  // TODO: If fed random data, this actually yields
  // an average of about -0.15, figure out why
  // (expected, zero. if fed predictable data, then
  // average can be made easily to be 0, 1, -1, as expected).


//  if (abs(result) > 0.7) {
//    cerr << "              ";
//  }
//    cerr << result << '\n';
//  cerr << result << '\n';
  if (!(result >= -1.0 && result <= 1.0)) {
    cerr << "Got result " << result << '\n';
  }
  assert(result >= -1.0 && result <= 1.0);

  return result;
}

vector<float>::iterator Sync::receiveAudioAndSync(vector<float> &samples) {
  // Append samples to buffer
  // TODO: just use a rotating buffer of a fixed float array
  buffer.insert(buffer.end(), samples.begin(), samples.end());

  while (buffer.size() >= bufferStart() + cfg->sync.syncChipSize) {
    Spectrum spectrum(buffer.data() + bufferStart(), SAMPLE_RATE, cfg->sync.syncChipSize);
    complex<float> bucketVals[cfg->sync.numSyncChannels];

    copyBucketVals(spectrum, bucketVals);
    metaBuffer.push_back(detectMatch(bucketVals));

    fftSampleIndex++;
  }

  if (false) {
    float tot = 0;
    for (int i = 0; i < metaBuffer.size(); i++) {
      tot += metaBuffer[i];
    }

    cerr << (tot / metaBuffer.size()) << '\n';
  }

  // Look for sync...

  // Discard head of buffer if no longer needed
  //if (buffer.size() > MAX_BUFFER_SAMPLES) {
  //  deque<float>::iterator head = buffer.begin();
  //  deque<float>::iterator newHead = head + (buffer.size() - MAX_BUFFER_SAMPLES);
  //  buffer.erase(head, newHead);
  //}

  // No sync yet
  return samples.end();
}

int Sync::bufferStart() {
  return (int) (samplesPerMetaSample * fftSampleIndex);
}

