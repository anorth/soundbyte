#ifndef _SYNC_H_
#define _SYNC_H_

#include <complex>
#include <deque>
#include <vector>

#include "config.h"

class Spectrum;

class Sync {
public:
  Sync(Config* cfg);
  ~Sync();

  /**
   * Generates a sync signal into target.
   */
  void generateSync(std::vector<float> &target);

  /**
   * Receives the next segment of audio and attempt to synchronise with this and
   * previous samples.
   * 
   * If sync is successful, returns an iterator addressing the first sample after
   * sync and resets state. If sync fails, returns samples.end() and maintains state
   * to continue processing with the next contiguous samples.
   */
  std::vector<float>::iterator receiveAudioAndSync(std::vector<float> &samples);

  /**
   * Resets internal sync state. 
   */
  void resetSync();

private:
  Config* cfg;

  float samplesPerMetaSample;
  int fftSampleIndex;

  //std::deque<float> buffer;

  std::vector<float> buffer;

  int bufferStart();

  void copyBucketVals(Spectrum &spectrum, std::complex<float> *out);

  std::complex<float> *pattern;
  float detectMatch(std::complex<float> *bucketVals);

  float bitPatternAbs;


  std::deque<float> metaBuffer;
};

#endif

