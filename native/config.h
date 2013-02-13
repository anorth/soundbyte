#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef struct {
  int numSyncChannels;
  int syncBaseBucket;
  int chipsPerSyncPulse;
  int numCyclesAsReadyPulses;
  float signalFactor;
  int detectionSamplesPerChip;
  float misalignmentTolerance;
  int syncChipSize;
} SyncConfig;

typedef struct {
  int baseFrequency;
  int baseBucket;
  int channelWidth;
  int channelSpacing;
  int chipRate;
  int chipSamples;
  int numChannels;

  SyncConfig sync;
} Config;

#endif
