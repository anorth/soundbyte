#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef struct {
  int numChannels;
  int baseBucket;
  int channelSpacing;
  int chipsPerSyncPulse;
  int numCyclesAsReadyPulses;
  float signalFactor;
  int detectionSamplesPerChip;
  float misalignmentTolerance;
  int chipSize;
  int longMetaBucket;
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
