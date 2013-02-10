#include "audio.h"

#include <iostream>
#include <vector>

#include "kiss_fftr.h"

using namespace std;

/** Decodes a buffer of little-endian PCM-16 into waveform amplitudes. */
void decodePcm16(char *buffer, int buflen, vector<float> &target) {
  target.reserve(buflen / 2);
  for (int i = 0; i < buflen; i += 2) {
    short *ps = (short *)(buffer + i);
    target[i/2] = (float)*ps + 0.5f / PCM_QUANT;
  }
}



//# Encodes waveform amplitudes as a little-endian PCM-16 stream
//def encodePcm(waveform):
//  shorts = [math.floor(PCM_QUANT * f) for f in waveform]
//  return struct.pack("<%dh" % len(shorts), *shorts)





