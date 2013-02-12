#include "scom.h"

#include "audio.h"
#include "assigners.h"
#include "codecs.h"
#include "config.h"
#include "constants.h"
#include "decoder.h"
#include "packeter.h"
#include "sync.h"

#include <cassert>
#include <vector>

#include <iostream>

using namespace std;

const char *HELLO = "Hello from C++";

static Config initCfg() {
  int base = 16000;
  int chipRate = 50;
  int chipSamples = SAMPLE_RATE / chipRate;

  Config cfg;
  cfg.baseFrequency = base;
  cfg.baseBucket = base * chipSamples / SAMPLE_RATE;
  cfg.channelSpacing = 2;
  cfg.numChannels = 8;
  cfg.chipRate = chipRate;

  int syncrate = 200;
  cfg.sync.chipsPerSyncPulse = 2;
  cfg.sync.numCyclesAsReadyPulses = 1;
  cfg.sync.signalFactor = 1.0;
  cfg.sync.detectionSamplesPerChip = 4;
  cfg.sync.misalignmentTolerance = 0.15;
  cfg.sync.syncChipSize = SAMPLE_RATE / syncrate;

  return cfg;
}

static Config cfg;
static Sync *syncer = 0;
static Codec *codec = 0;
static Assigner *assigner = 0;
static Packeter *packeter = 0;
static Decoder *decoder = 0;

void init() {
  cfg = initCfg();
  syncer = new Sync(&cfg);
  codec = new IdentityCodec();
  assigner = new CombinadicAssigner(cfg.numChannels);
  packeter = new Packeter(codec, assigner);
  decoder = new Decoder(syncer, packeter);
}

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  return 0;
}

void decodeAudio(char *buffer, int buflen) {
  std::vector<float> samples;
  decodePcm16(buffer, buflen, samples);

  decoder->receiveAudio(samples);
}

bool messageAvailable() {
  return decoder->messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return decoder->takeMessage(buffer, bufferCapacity);
}

///// Private /////



