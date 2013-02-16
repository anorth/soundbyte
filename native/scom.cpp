#include "scom.h"

#include "audio.h"
#include "assigners.h"
#include "codecs.h"
#include "config.h"
#include "constants.h"
#include "log.h"
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
  int base = 15000;
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
  cfg.sync.baseBucket = base * syncChipSize / SAMPLE_RATE;
  cfg.sync.channelSpacing = 2;
  cfg.sync.numChannels = cfg.numChannels;
  cfg.sync.chipsPerSyncPulse = 2;
  cfg.sync.numCyclesAsReadyPulses = 1;
  cfg.sync.signalFactor = 1.0;
  cfg.sync.detectionSamplesPerChip = 4;
  cfg.sync.misalignmentTolerance = 0.18;
  cfg.sync.chipSize = syncChipSize;
  cfg.sync.longMetaBucket = 2;

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
  ll(LOG_INFO, "SCOM", "Initialising scom %d", 3);
  cfg = initCfg();
  syncer = new Sync(&cfg.sync);
  codec = new IdentityCodec(1); // some number, can be 1, can be 50.
  //codec = new RsCodec(
  //  20, // encoded size (symbols)
  //  10, // message size (symbols)
  //  8   // symbols size (bits)
  //  );
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
    //for (int i = 0; i < 10; ++i) {
    //  cerr << encoded[i] << ", ";
    //}
    //cerr << endl;
    //for (int i = 0; i < 10; ++i) {
    //  cerr << int(waveform[i]) << ", ";
    //}
    //cerr << endl;
    return requiredCapacity;
  } else {
    cerr << "Oops" << '\n';
    return 0;
  }
}

void decodeAudio(char *buffer, int buflen) {
  cerr << "decode audio" << endl;
  std::vector<float> samples;
  decodePcm16(buffer, buflen, samples);
  //cerr << "Decoded PCM" << endl;
  //for (int i = 0; i < 10; ++i) {
  //  cerr << int(buffer[i]) << ", ";
  //}
  //cerr << endl;
  //for (int i = 0; i < 10; ++i) {
  //  cerr << samples[i] << ", ";
  //}
  //cerr << endl;

  receiver->receiveAudio(samples);
}

bool messageAvailable() {
  return receiver->messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return receiver->takeMessage(buffer, bufferCapacity);
}

///// Private /////



