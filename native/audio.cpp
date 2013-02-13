#include "audio.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "constants.h"
#include "spectrum.h"
#include "util.h"
#include "kiss_fftr.h"

using namespace std;

static const float PCM_QUANT = 32767.5;

void silence(int nsamples, vector<float> &target) {
  target.insert(target.end(), nsamples, 0);
}

void sinewave(float freq, int nsamples, vector<float> &target) {
  target.reserve(target.size() + nsamples);
  vector<float>::iterator tit = target.end();
  float twoPiOnRate = 2.0 * M_PI * freq / float(SAMPLE_RATE);
  for (int i = 0; i < nsamples; ++i) {
    target.insert(tit, sin(i * twoPiOnRate));
    ++tit;
  }
}

void sinewaves(vector<float> &frequencies, int nsamples, vector<float> &target) {
  //cerr << "IFFT for " << frequencies << endl;
  kiss_fft_cpx buckets[nsamples / 2 + 1];
  bzero(buckets, sizeof(buckets));
  for (vector<float>::iterator it = frequencies.begin(); it != frequencies.end(); ++it) {
    int bucket = (int)((*it / SAMPLE_RATE) * nsamples);
    buckets[bucket].r = 1.0;
    buckets[bucket].i = 0.0;
   //cerr << bucket << ", ";
  }
  //cerr << endl;

  vector<float> intermediate(nsamples);

  // Allocate FFT memory on stack
  size_t fftMemNeeded = 0;
  kiss_fftr_alloc(nsamples, 1, 0, &fftMemNeeded);
  assert(fftMemNeeded > 0);
  char fftBuffer[fftMemNeeded];

  kiss_fftr_cfg ifft = kiss_fftr_alloc(nsamples, 1, fftBuffer, &fftMemNeeded);
  assert(ifft == (void*)fftBuffer);
  kiss_fftri(ifft, buckets, intermediate.data());

  normalize(intermediate);
  // TODO: fades
  //cerr << intermediate << endl;

  // For testing
  //Spectrum(intermediate.data(), SAMPLE_RATE, nsamples);

  target.insert(target.end(), intermediate.begin(), intermediate.end());
}

void normalize(vector<float> &waveform) {
  float divisor = 0.0;
  vector<float>::iterator it;
  for (it = waveform.begin(); it != waveform.end(); ++it) {
    float d = abs(*it);
    if (d > divisor) { divisor = d; }
  }

  if (divisor > 0.0) {
    for (it = waveform.begin(); it != waveform.end(); ++it) {
      *it = *it / divisor;
    }
  }
}

void limit(vector<float> &waveform) {
  vector<float>::iterator it;
  for (it = waveform.begin(); it != waveform.end(); ++it) {
    if (*it > 1.0) { *it = 1.0; }
    if (*it < -1.0) { *it = -1.0; }
  }
}

void buildWaveform(vector<bool> &chip, float base, float channelSpacing, int nsamples, 
    vector<float> &target) {
  vector<float> freqs;
  for (int i = 0; i < chip.size(); ++i) {
    if (chip[i]) {
      freqs.push_back(base + channelSpacing * i);
    }
  }
  sinewaves(freqs, nsamples, target);
}

void window(vector<float> &waveform) {
  float constant = 2.0 * M_PI / (waveform.size() - 1);
  for (int i = 0; i < waveform.size(); ++i) {
    waveform[i] = waveform[i] * 0.5 * (1.0 - cos(i * constant));
  }
}

void fadein(vector<float> &waveform, int nsamples) {
  assert(nsamples <= waveform.size());
  for (int i = 0; i < nsamples; ++i) {
    waveform[i] *= ((float)i) / nsamples;
  }
}

void fadeout(vector<float> &waveform, int nsamples) {
  assert(nsamples <= waveform.size());
  for (int i = waveform.size() - nsamples; i < waveform.size(); ++i) {
    waveform[i] *= (waveform.size() - i) / (float)nsamples;
  }
}

void decodePcm16(char *buffer, int buflen, vector<float> &target) {
  target.reserve(target.size() + buflen / 2);
  for (int i = 0; i < buflen; i += 2) {
    short *ps = (short *)(buffer + i);
    target.push_back((float)*ps + 0.5f / PCM_QUANT);
  }
}

void encodePcm16(vector<float> &samples, char *buffer) {
  for (int i = 0; i < samples.size(); ++i) {
    short s = floor(PCM_QUANT * samples[i]);
    ((short *)buffer)[i] = s;
  }
}

