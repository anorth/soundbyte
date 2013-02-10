#include "spectrum.h"

using namespace std;

static int bucket(float f);

Spectrum::Spectrum(vector<float> &samples) {
// FIXME
}

int Spectrum::size() {
  return buckets.size();
}

float Spectrum::bucketWidth() {
  return rate / buckets.size();
}

complex Spectrum::at(int bucket) {
  return buckets.at(bucket);
}

complex Spectrum::at(float f) {
  return at(bucket(f));
}

float Spectrum::amplitude(int bucket) {
  return buckets[bucket].abs();
}

float Spectrum::amplitude(float frequency) {
  return amplitude(bucket(f));
}

float Spectrum::power(int bucket) {
  float a = amplitude(bucket);
  return a * a;
}

float Spectrum::power(float frequency) {
  return power(bucket(f));
}

int Spectrum::bucket(float f) {
  int index = f / bucketWidth();
  assert(floor(f) == ceil(f));
  return index;
}
