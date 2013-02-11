#include "sync.h"
#include "constants.h"

#include <cmath>
#include <cassert>

using namespace std;

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 2; // 2s

Sync::Sync(SyncConfig* cfg) {
  this->cfg = cfg;

  samplesPerMetaSample = (float) cfg->chipSize / cfg->detectionSamplesPerChip;
  pattern = new complex<float>[cfg->numchans];

  resetSync();

  assert(cfg->numchans % 2 == 0);
  bitPatternAbs = sqrt((float)cfg->numchans/2);
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
  for (int i = 0; i < cfg->numchans; i++) {
    out[i] = spectrum.at(cfg->baseBucket + i * cfg->spacing);
  }
}

float Sync::detectMatch(complex<float> *bucketVals) {
  float bucketSum = 0.0;
  float result = 0;
  for (int i = 0; i < cfg->numchans; i++) {
    float a = abs(bucketVals[i]);
    bucketSum += a;
    if (i % 2 == 0) {
      result += a;
    }
  }
  result /= sqrt(bucketSum);
  result /= bitPatternAbs;

  result *= 0.999;
  result += 0.0005;

  result = result * 2 - 1;

  assert(result > -1.0 && result < 1.0);

  return result;
}

vector<float>::iterator Sync::receiveAudioAndSync(vector<float> &samples) {
  // Append samples to buffer
  // TODO: just use a rotating buffer of a fixed float array
  buffer.insert(buffer.end(), samples.begin(), samples.end());

  while (buffer.size() >= bufferStart() + cfg->chipSize) {
    Spectrum spectrum(samples.data() + bufferStart(), SAMPLE_RATE, cfg->chipSize);
    complex<float> bucketVals[cfg->numchans];

    copyBucketVals(spectrum, bucketVals);
    metaBuffer.push_back(detectMatch(bucketVals));

    fftSampleIndex++;
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

