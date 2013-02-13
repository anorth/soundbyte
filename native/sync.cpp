#include "sync.h"

#include "config.h"
#include "constants.h"
#include "spectrum.h"

//#include <iostream>
#include <cmath>
#include <cassert>

using namespace std;

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 2; // 2s

//void printArray(float* values, int length) {
//  cerr << "\n[";
//  for (int i = 0; i < length; i++) {
//    if (i > 0) cerr << ',';
//    cerr << values[i];
//  }
//  cerr << "]\n";
//}

Sync::Sync(SyncConfig* cfg) : cfg(cfg) {
  samplesPerMetaSample = (float) cfg->chipSize / cfg->detectionSamplesPerChip;

  assert(cfg->numChannels % 2 == 0);
  bitPatternAbs = sqrt((float)cfg->numChannels/2);

  metaSamplesPerPulse = cfg->detectionSamplesPerChip * cfg->chipsPerSyncPulse;
  //cerr << metaSamplesPerPulse << "<------------\n";
  metaSamplesPerCycle = 2 * metaSamplesPerPulse;
  readyRepeats = 2 * cfg->chipsPerSyncPulse;
  pulseSamples = cfg->chipsPerSyncPulse * cfg->chipSize;

  resetSync();

  //cerr << "VALS " << cfg->baseBucket << ' '
      //<< cfg->numChannels << ' ' << cfg->channelSpacing << '\n';
}

Sync::~Sync() {
}

void Sync::generateSync(vector<float> &target) {
  
}

void Sync::resetSync() {
  fftSampleIndex = 0;
  syncOffset = 0;
  state = -1;
  buffer.clear();
  shortMetaBuffer.clear();
  metaBuffer.clear();
  // TODO: reserve certain size in buffers.
}

vector<float>::iterator Sync::receiveAudioAndSync(vector<float> &samples) {
  //cerr << "\n\n\n===========\nCALLED with " << samples.size() << " samples\n";
  // Append samples to buffer
  // TODO: just use a rotating buffer of a fixed float array
  buffer.insert(buffer.end(), samples.begin(), samples.end());

  // need extra chipSize at the end for possible alignment signal,
  // which is 1 long cycle + 1 chip in size (sync signals need to
  // have an odd number of pulses).
  while (buffer.size() >= bufferStart() + pulseSamples + cfg->chipSize) {
    //Spectrum spectrum(buffer.data() + bufferStart(), SAMPLE_RATE, pulseSamples);
    Spectrum spectrum(buffer.data() + bufferStart(), SAMPLE_RATE, cfg->chipSize);
    complex<float> bucketVals[cfg->numChannels];
    //copyBucketVals(spectrum, cfg->chipsPerSyncPulse, bucketVals);
    copyBucketVals(spectrum, 1, bucketVals);
    shortMetaBuffer.push_back(detectMatch(bucketVals));

    fftSampleIndex++;

    if (fftSampleIndex >= cfg->detectionSamplesPerChip) {
      float tot = 0;
      for (int j = fftSampleIndex - cfg->detectionSamplesPerChip; j < fftSampleIndex; j++) {
        tot += shortMetaBuffer[j];
      }
      metaBuffer.push_back(tot / cfg->detectionSamplesPerChip);
    }
  }

  if (false) {
    float tot = 0;
    for (int i = 0; i < metaBuffer.size(); i++) {
      tot += metaBuffer[i];
    }
    //cerr << (tot / metaBuffer.size()) << '\n';
  }

  //cerr << "\nsyncOffset " << syncOffset << ' ' << 2*pulseSamples << '\n';
  int longMetaSpectrumLength = metaSamplesPerCycle * cfg->longMetaBucket;

  while (true) {
    int metaBufferStart = (int) (syncOffset / samplesPerMetaSample);
    //cerr << "\n---------loop " << syncOffset << ", " << metaBufferStart << '\n';
    assert(metaBufferStart < metaBuffer.size());

    if (metaBuffer.size() < metaBufferStart + longMetaSpectrumLength) {
      // No sync yet
      return samples.end();
    }

    
    float *longMetaSignal = metaBuffer.data() + metaBufferStart;
    //printArray(metaBuffer.data(), metaBuffer.size());
    Spectrum longMetaSpectrum(longMetaSignal, -1, longMetaSpectrumLength);
    int longPartALength = (cfg->longMetaBucket - 1) * metaSamplesPerCycle;
    int longPartBLength = longMetaSpectrumLength - longPartALength;
    assert(longPartBLength > 0);
    assert(longPartALength >= longPartBLength);
    assert(longPartALength + longPartBLength == longMetaSpectrumLength);
    assert(longPartALength >= metaSamplesPerCycle);
    Spectrum longMetaSpectrumA(longMetaSignal, -1, longPartALength);
    Spectrum longMetaSpectrumB(longMetaSignal + longPartALength, -1, longPartBLength);

    //cerr << "LEN " << longPartALength << ' ' << longPartBLength << ' ' << longMetaSpectrumLength << '\n';

    float misalignment;

    // final ready pulse is 2 long cycles in size + 1 chip
    if (state >= 1) {
      int readySignalStart = 
        syncOffset + ((cfg->longMetaBucket - 1) * 2 + 1) * pulseSamples;
      //cerr << "SYNC OFFSET " << syncOffset << ' ' << readySignalStart << '\n';
      int shortMetaSpectrumLength = metaSamplesPerCycle;
      float readyBuffer[shortMetaSpectrumLength];
      for (int i = 0; i < shortMetaSpectrumLength; i++) {
        //buffer.size() >= bufferStart() + cfg->chipSize) {
        Spectrum spectrum(
          buffer.data() + readySignalStart + (int)(samplesPerMetaSample*i),
          SAMPLE_RATE, cfg->chipSize);
        complex<float> bucketVals[cfg->numChannels];

        copyBucketVals(spectrum, 1, bucketVals);
        readyBuffer[i] = detectMatch(bucketVals);
      }
      Spectrum shortMetaSpectrumB(readyBuffer, -1, shortMetaSpectrumLength);
      //printArray(readyBuffer, shortMetaSpectrumLength);
      //printArray(shortMetaBuffer.data(), shortMetaBuffer.size());
      //printArray(metaBuffer.data(), metaBuffer.size());

      if (state == 2) {
        int resultShortB = getAlignment(shortMetaSpectrumB, cfg->chipsPerSyncPulse,
            1, 1, NULL);
        if (resultShortB == 1) {
          //logging.debug('state 2 ALIGNED')
          // XXX return actual alignment
          int offset = readySignalStart + (int)(samplesPerMetaSample*shortMetaSpectrumLength) + cfg->chipSize;
          int inputOffset = samples.size() - (buffer.size() - offset);
          //cerr << "\n\nSUCCESS " << offset << ' ' <<  inputOffset << "\n\n";
          assert(false);
          return samples.end();
        } else {
          state = -1;
        }
      }
      
      if (state == 1) {
        int resultLongA = getAlignment(longMetaSpectrumA,
            cfg->longMetaBucket - 1, 1, cfg->chipsPerSyncPulse, NULL);
        int resultLongB = getAlignment(longMetaSpectrumB,
            1, 1, cfg->chipsPerSyncPulse, NULL);
        float chipMisalignment;
        int resultShortB = getAlignment(shortMetaSpectrumB,
            cfg->chipsPerSyncPulse, 1, 1, &chipMisalignment);
        //logging.debug('\n|| %s %s %s' % (resultLongA, resultLongB, resultShortB))
        if (resultLongA == 1 && resultLongB < 1 && resultShortB >= 0) {
          //logging.debug('========= %s %s %s' % (resultLongA, resultLongB, resultShortB))
          //logging.debug('state 1 ALIGNED')
          state = 2;
          misalignment = chipMisalignment / cfg->chipsPerSyncPulse;
          //cerr << "STATE 2 " << misalignment << '\n';
        }
        //cerr << "XXstate " << state << ',' << resultLongA << ' ' << resultLongB 
            //<< ' ' << resultShortB << '\n';
      }
    }

    if (state < 2) {
      // Check for, and align to, the long signal
      //#logging.debug('L')
      state = getAlignment(longMetaSpectrum,
          cfg->longMetaBucket, state, cfg->chipsPerSyncPulse, &misalignment);
      //cerr << "newstate " << state << ' ' << misalignment << '\n';
    }
        //cerr << "misalignmetn " << misalignment << '\n';

    if (state < 0) {
      misalignment = 0;
    }

    //if True or state < 2:
    //  alignedAmount = int(signalCycleSize * (1 - misalignment))
    //else:
    //  alignedAmount = signalCycleSize * (metaSignalBucket - 1) + pulseSize
    //  logging.debug('aligning +%d cycles' % (metaSignalBucket - 1))

    int alignedAmount = (int)(2*pulseSamples * (1 - misalignment));

    syncOffset += alignedAmount;

    //cerr << state << ' ';

  }
}

float largestOther(Spectrum &spectrum, int notBucket) {
  // skip bucket 0 and 'notBucket'
  float max = 0;
  for (int i = 1; i < spectrum.size(); i++) {
    if (i != notBucket) {
      float v = abs(spectrum.at(i));
      if (v > max) max = v;
    }
  }
  return max;
}

#define DBG true
#define PI2 6.28318530718f

int Sync::getAlignment(Spectrum &spectrum, int bucket, int state, int numChips,
    float *misalignmentOut) {
  char label = numChips == 1 ? '!' : '.'; // for debugging
  
  complex<float> bucketValue = spectrum.at(bucket);
  float resultAmplitude = abs(bucketValue);
  float largestRemaining = largestOther(spectrum, bucket);
  //logging.debug('REMAINING: %s' % largestRemaining)
  float misalignment = 0;
  int result;

  //logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))
  if (abs(bucketValue) > cfg->signalFactor * largestRemaining) {
    //logging.debug('%s, %s' % (abs(bucketValue), largestRemaining))
    misalignment = arg(bucketValue) / PI2;
    assert(misalignment >= -0.5 && misalignment <= 0.5);
    float misalignmentPerChip = misalignment * numChips;
    if (state == -1) {
      //logging.debug('[%s]' % label)
      result = 0; // # don't consider misalignment on first go
    } else if (abs(misalignmentPerChip) <= cfg->misalignmentTolerance) {
      result = 1;
      if (DBG) {
        //logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))
      }
      //logging.debug('(%s %.3f)' % (label, abs(misalignmentPerChip)))
    } else {
      //#logging.debug('<%s>' % label, False)
      //logging.debug('<%s:%s>' % (label, misalignmentPerChip))
      //#logging.debug('MISALIGNED %s %.2f' % (label, misalignmentPerChip))
      result = 0; // couldn't have been a good signal
    }
  } else {
    result = -1;
    //logging.debug('%s' % label)
    if (DBG) {
//      logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))
    }
  }

  if (misalignmentOut) *misalignmentOut = misalignment;
  return result;
}


int Sync::bufferStart() {
  return (int) (samplesPerMetaSample * fftSampleIndex);
}


float Sync::detectMatch(complex<float> *bucketVals) {
  // Computes dot-product of normalized bucketVals with normalized bit pattern.

  float bucketSum = 0.0;
  float result = 0;

  for (int i = 0; i < cfg->numChannels; i++) {
    float a = abs(bucketVals[i]);
    // Keep track of sum of squares, to normalise at end
    bucketSum += a*a;

    // Sync match bit pattern is 1,0,1,0,1,0...
    // Therefore add every 2nd one
    if (i % 2 == 0) {
      result += a;
    }
  }
  // Normalize
  result /= sqrt(bucketSum);
  result /= bitPatternAbs;

  // can rarely get NaNs, guard against
  result *= 0.99999;
  result += 0.000005;

  // Convert to linear correlation
  result = 1.57079632679f - 2*acos(result);

  // Map to range -1,1, with sinewave correlation
  result = sin(result);
  //result = result / 1.57079632679f;

  // TODO: If fed random data, this actually yields
  // an average of about -0.15, figure out why
  // (expected, zero. if fed predictable data, then
  // average can be made easily to be 0, 1, -1, as expected).
  // Not sure if it happens with random bucket values,
  // or if it's random time domain data that causes some
  // bias after FFT.


//  if (abs(result) > 0.7) {
//    cerr << "              ";
//  }
//    cerr << result << '\n';
//  cerr << result << '\n';
  if (!(result >= -1.0 && result <= 1.0)) {
    //cerr << "Got result " << result << '\n';
  }
  assert(result >= -1.0 && result <= 1.0);

  return result;
}

void Sync::copyBucketVals(Spectrum &spectrum, int numChipsInSample, complex<float> *out) {
  for (int i = 0; i < cfg->numChannels; i++) {
    out[i] = spectrum.at(
        numChipsInSample * (cfg->baseBucket + (i * cfg->channelSpacing)));
  }
}
