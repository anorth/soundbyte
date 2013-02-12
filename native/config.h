#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef struct {
  int numSyncChannels;
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
  int channelSpacing;
  int chipRate;
  int numChannels;

  SyncConfig sync;
} Config;

#endif
