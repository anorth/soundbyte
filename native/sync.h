#ifndef _SYNC_H_
#define _SYNC_H_

#include <complex>
#include <deque>
#include <vector>

#include "config.h"
#include "stream.h"

class Spectrum;

class Sync {
public:
  Sync(SyncConfig* cfg, Stream<float>& source);
  ~Sync();

  /**
   * Generates a sync signal into target.
   */
  void generateSync(std::vector<float> &target);

  int getState();

  bool sync();

private:
  int bufferStart();
  void copyBucketVals(Spectrum &spectrum, int numChipsInSample, std::vector<std::complex<float> > &out);
  float detectMatch(std::vector<std::complex<float> > &bucketVals);
  int getAlignment(Spectrum &spectrum, int bucket, int state, int numChips,
      float *misalignmentOut);
  void createSyncCycles(int chipsPerPulse, int cycles, std::vector<float> &target);
  void reset(bool consume);

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
  Stream<float>& source;
  std::vector<float> shortMetaBuffer;
  std::vector<float> metaBuffer;
};

#endif

