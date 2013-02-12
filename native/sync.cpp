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
    out[i] = spectrum.at(cfg->baseBucket + i * cfg->channelSpacing);
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
//  cerr << result << ' ' << bitPatternAbs << ' ' << bucketSum << ' ';
  // Normalize
  result /= sqrt(bucketSum);
  result /= bitPatternAbs;

  // Just in case
  //result *= 0.999;
  //result += 0.0005;

  result = 1.57079632679f - 2*acos(result);
  //cerr << result << '\n';
  //// Map to range -1,1
  //result = sin(result * 2 - 1.57079632679f);
  result = sin(result);


 // if (abs(result) > 0.5) {
 //   cerr << "             ";
 // }
  //cerr << result << '\n';
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

  float tot = 0;
  // WTF compiler errors
  //for (vector<float>::iterator it = metaBuffer.begin(); it != metaBuffer.end(); ++it) {
  for (int i = 0; i < metaBuffer.size(); i++) {
    tot += metaBuffer[i];
  }

  cerr << (tot / metaBuffer.size()) << '\n';

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

