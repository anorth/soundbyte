#include "scom.h"

#include "audio.h"
#include "assigners.h"
#include "codecs.h"
#include "config.h"
#include "constants.h"
#include "packeter.h"
#include "receiver.h"
#include "sender.h"
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
  cfg.channelWidth = SAMPLE_RATE / chipSamples;
  cfg.channelSpacing = 2;
  cfg.numChannels = 8;
  cfg.chipRate = chipRate;
  cfg.chipSamples = chipSamples;

  int syncrate = 200;
  int syncChipSize = SAMPLE_RATE / syncrate;
  cfg.sync.syncBaseBucket = base * syncChipSize / SAMPLE_RATE;
  cfg.sync.numSyncChannels = cfg.numChannels;
  cfg.sync.chipsPerSyncPulse = 2;
  cfg.sync.numCyclesAsReadyPulses = 1;
  cfg.sync.signalFactor = 1.0;
  cfg.sync.detectionSamplesPerChip = 4;
  cfg.sync.misalignmentTolerance = 0.15;
  cfg.sync.syncChipSize = syncChipSize;

  return cfg;
}

static Config cfg;
static Sync *syncer = 0;
static Codec *codec = 0;
static Assigner *assigner = 0;
static Packeter *packeter = 0;
static Sender *sender = 0;
static Receiver *receiver = 0;

void scomInit() {
  cfg = initCfg();
  syncer = new Sync(&cfg);
  codec = new IdentityCodec();
  assigner = new CombinadicAssigner(cfg.numChannels);
  packeter = new Packeter(&cfg, codec, assigner);
  sender = new Sender(&cfg, syncer, packeter);
  receiver = new Receiver(&cfg, syncer, packeter);
}

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  vector<char> message(payload, payload + payloadLength);
  vector<float> encoded;
  sender->encodeMessage(message, encoded);

  int requiredCapacity = encoded.size() * sizeof(short) / sizeof(char);
  if (waveformCapacity > requiredCapacity) {
    encodePcm16(encoded, waveform);
    cerr << "Sent " << requiredCapacity << " bytes for " << encoded.size() << " samples" << endl;
    return requiredCapacity;
  } else {
    return 0;
  }
}

void decodeAudio(char *buffer, int buflen) {
  std::vector<float> samples;
  decodePcm16(buffer, buflen, samples);

  receiver->receiveAudio(samples);
}

bool messageAvailable() {
  return receiver->messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return receiver->takeMessage(buffer, bufferCapacity);
}

///// Private /////



