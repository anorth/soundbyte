#include "spectrum.h"

#include <cassert>
#include "kiss_fftr.h"

using namespace std;

Spectrum::Spectrum(vector<float> &samples, int rate) :
    rate(rate),
    buckets(samples.size() / 2 + 1) {
  kiss_fftr_cfg fft = kiss_fftr_alloc(samples.size(), 0, 0, 0);
  kiss_fft_cpx kissBuckets[buckets.size()];

  kiss_fftr(fft, samples.data(), kissBuckets);
  for (int i = 0; i < buckets.size(); ++i) {
    buckets[i] = complex<float>(kissBuckets[i].r, kissBuckets[i].i);
  }

  kiss_fft_cleanup();
  kiss_fftr_free(fft);
}

int Spectrum::size() {
  return buckets.size();
}

float Spectrum::bucketWidth() {
  return rate / buckets.size();
}

complex<float> Spectrum::at(int bucket) {
  return buckets.at(bucket);
}

complex<float> Spectrum::at(float f) {
  return at(bucket(f));
}

float Spectrum::amplitude(int bucket) {
  return abs(buckets[bucket]);
}

float Spectrum::amplitude(float frequency) {
  return amplitude(bucket(frequency));
}

float Spectrum::power(int bucket) {
  float a = amplitude(bucket);
  return a * a;
}

float Spectrum::power(float frequency) {
  return power(bucket(frequency));
}

int Spectrum::bucket(float f) {
  int index = f / bucketWidth();
  assert(floor(f) == ceil(f));
  return index;
}
