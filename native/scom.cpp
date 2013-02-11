#include "scom.h"

#include "audio.h"
#include "constants.h"
#include "decoder.h"

#include <cassert>
#include <vector>

#include <iostream>

using namespace std;

const char *HELLO = "Hello from C++";

static SyncConfig initCfg() {
  int syncrate = 200;
  int base = 16000;
  int spacing = 2;
  int numSyncChans = 8;

  int syncChipSamples = SAMPLE_RATE / syncrate;

  SyncConfig cfg;
  cfg.baseBucket = base * syncChipSamples / SAMPLE_RATE;
  cfg.spacing = spacing;
  cfg.numchans = numSyncChans;
  cfg.chipsPerSyncPulse = 2;
  cfg.numCyclesAsReadyPulses = 1;
  cfg.signalFactor = 1.0;
  cfg.detectionSamplesPerChip = 4;
  cfg.misalignmentTolerance = 0.15;
  cfg.chipSize = syncChipSamples;

  return cfg;
}

static SyncConfig cfg = initCfg();

static Sync syncer(&cfg);

static Decoder decoder(&syncer);

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  return 0;
}

void decodeAudio(char *buffer, int buflen) {
  std::vector<float> samples;
  decodePcm16(buffer, buflen, samples);

  decoder.receiveAudio(samples);
}

bool messageAvailable() {
  return decoder.messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return decoder.takeMessage(buffer, bufferCapacity);
}

///// Private /////



