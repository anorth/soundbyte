#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <vector>

const float PCM_QUANT = 32767.5;

void decodePcm16(char *buffer, int buflen, std::vector<float> &target);

#endif
