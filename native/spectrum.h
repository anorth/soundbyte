#ifndef _SPECTRUM_H_
#define _SPECTRUM_H_

#include <complex>
#include <vector>

class Spectrum {
public:
  /** Performs FFT */
  //Spectrum(std::vector<float> &samples, int rate);
  Spectrum(float *samples, int rate, int size);

  /** Number of buckets. */
  int size();

  /** Width of a bucket. */
  float bucketWidth();

  /** Value at bucket. */
  std::complex<float> at(int bucket);

  /** Value at bucket centred on frequency. */
//  std::complex<float> at(float frequency);

  /** Amplitude of bucket. */
  float amplitude(int bucket);

  /** Amplitude of bucket centred on frequency. */
 // float amplitude(float frequency);

  /** Power of bucket. */
  float power(int bucket); 

  /** Power of bucket centred on frequency. */
  float power(float frequency); 

private:
  Spectrum();
  int bucket(float f);

  int rate;
  std::vector<std::complex<float> > buckets;

};

#endif
