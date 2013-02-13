#ifndef _SYNC_H_
#define _SYNC_H_

#include <complex>
#include <deque>
#include <vector>

#include "config.h"

class Spectrum;

class Sync {
public:
  Sync(SyncConfig* cfg);
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
  void reset();

private:
  int bufferStart();
  void copyBucketVals(Spectrum &spectrum, int numChipsInSample, std::complex<float> *out);
  float detectMatch(std::complex<float> *bucketVals);
  int getAlignment(Spectrum &spectrum, int bucket, int state, int numChips,
      float *misalignmentOut);

  // Parameters
  SyncConfig* cfg;
  float samplesPerMetaSample;
  float bitPatternAbs;
  int readyRepeats;
  int pulseSamples;
  int metaSamplesPerPulse;
  int metaSamplesPerCycle;

  // State
  int state;
  int fftSampleIndex;
  int syncOffset; // Sync offset in samples
  std::vector<float> buffer;
  std::vector<float> shortMetaBuffer;
  std::vector<float> metaBuffer;
};

#endif

