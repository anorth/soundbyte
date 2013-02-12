#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <vector>

/** Fills a vector with nsamples of silence. */
void silence(int nsamples, std::vector<float> &target);

/** Generates nsamples of a sine wave. */
void sinewave(float freq, int nsamples, std::vector<float> &target);

/** Generates nsamples of a sinewave at each frequency. */
void sinewaves(std::vector<float> &frequencies, int nsamples, std::vector<float> &target);

/** Normalizes a waveform, scaling values to a range +/- 1.0. */
void normalize(std::vector<float> &waveform);

/** Clips a waveform to +/- 1.0 */
void limit(std::vector<float> &waveform);

/** Builds a signal with energy at each frequency corresponding to a non-zero item in chip */
void buildWaveform(std::vector<bool> &chip, float base, float spacing, int nsamples,
    std::vector<float> &target);

/** Applies a Hann window to a waveform. */
void window(std::vector<float> &waveform);

/** Fades in a waveform over some samples. */
void fadein(std::vector<float> &waveform, int nsamples);

/** Fades out a waveform over some samples. */
void fadeout(std::vector<float> &waveform, int nsamples);

/** Decodes a little-endian PCM-16 buffer into a waveform. */
void decodePcm16(char *buffer, int buflen, std::vector<float> &target);

/** Encodes amplitudes into little-endian PCM-16 in buffer. */
void encodePcm16(std::vector<float> &samples, char *buffer);

#endif
