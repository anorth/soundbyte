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

static Config cfg;
static Sync *syncer = 0;
static Codec *codec = 0;
static Assigner *assigner = 0;
static Packeter *packeter = 0;
static Sender *sender = 0;
static Receiver *receiver = 0;


void scomInit() {
  scomInit(
    16000, //base
    50,    //rate
    2,     //spacing 
    8      //channels
    );
}
void scomInit(int base, int chipRate, int channelSpacing, int numChans) {
  ll(LOG_INFO, "SCOM", "Initialising scom %d", 3);

  int chipSamples = SAMPLE_RATE / chipRate;

  cfg.baseFrequency = base;
  cfg.baseBucket = base * chipSamples / SAMPLE_RATE;
  cfg.channelWidth = SAMPLE_RATE / chipSamples;
  cfg.channelSpacing = channelSpacing;
  cfg.numChannels = numChans;
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

  ll(LOG_INFO, "SCOM", "----");
  ll(LOG_INFO, "SCOM", "Base frequency %d", cfg.baseFrequency);
  ll(LOG_INFO, "SCOM", "Data rate %d", cfg.chipRate);
  ll(LOG_INFO, "SCOM", "Data channels %d", cfg.numChannels);
  ll(LOG_INFO, "SCOM", "Data spacing %d", cfg.channelSpacing);
  ll(LOG_INFO, "SCOM", "Sync metabucket %d", cfg.sync.longMetaBucket);
  ll(LOG_INFO, "SCOM", "Sync samp per chip %d", cfg.sync.detectionSamplesPerChip);
  ll(LOG_INFO, "SCOM", "Sync rate %d", SAMPLE_RATE / cfg.sync.chipSize);
  ll(LOG_INFO, "SCOM", "Sync channels %d", cfg.sync.numChannels);
  ll(LOG_INFO, "SCOM", "Sync spacing %d", cfg.sync.channelSpacing);
  ll(LOG_INFO, "SCOM", "----");

  syncer = new Sync(&cfg.sync);
  //codec = new IdentityCodec(7); // some number, can be 1, can be 50.
  codec = new RsCodec(
    20, // encoded size (symbols)
    10, // message size (symbols)
    8   // symbols size (bits)
    );
  assigner = new CombinadicAssigner(cfg.numChannels);
  packeter = new Packeter(&cfg, codec, assigner);
  sender = new Sender(&cfg, syncer, packeter);
  receiver = new Receiver(&cfg, syncer, packeter);
}

int encodeMessage(char* payload, int payloadLength, char *waveform, int waveformCapacity) {
  vector<char> message(payload, payload + payloadLength);
  vector<float> encoded;
  ll(LOG_INFO, "SCOM", "encode ");
  sender->encodeMessage(message, encoded);
  ll(LOG_INFO, "SCOM", "encodeDD ");

  int requiredCapacity = encoded.size() * sizeof(short) / sizeof(char);
  if (waveformCapacity > requiredCapacity) {
    ll(LOG_INFO, "SCOM", "pcm16 start ");
    encodePcm16(encoded, waveform);
    ll(LOG_INFO, "SCOM", "pcm16 end ");
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

int decodeAudio(char *buffer, int buflen) {
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

  return receiver->receiveAudio(samples);
}

bool messageAvailable() {
  return receiver->messageAvailable();
}

int takeMessage(char *buffer, int bufferCapacity) {
  return receiver->takeMessage(buffer, bufferCapacity);
}

///// Private /////



