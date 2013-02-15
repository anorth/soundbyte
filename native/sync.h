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
   * returns true if synced, and initial data in trailingSamplesOut.
   */
  bool receiveAudioAndSync(const std::vector<float> &samples,
      std::vector<float> &trailingSamplesOut);

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
  void createSyncCycles(int chipsPerPulse, int cycles, std::vector<float> &target);

  // Parameters
  SyncConfig* cfg;
  float samplesPerMetaSample;
  float bitPatternAbs;
  int readyRepeats;
  int pulseSamples;
  int metaSamplesPerPulse;
  int metaSamplesPerCycle;
  std::complex<float> **precomp;

  // State
  int state;
  int fftSampleIndex;
  int syncOffset; // Sync offset in samples
  std::vector<float> buffer;
  std::vector<float> shortMetaBuffer;
  std::vector<float> metaBuffer;
};

#endif

