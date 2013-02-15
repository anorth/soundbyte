#include "sync.h"

#include "config.h"
#include "constants.h"
#include "spectrum.h"
#include "audio.h"
#include "log.h"

#include <iostream>
#include <cmath>
#include <cassert>

using namespace std;

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 10; // num in seconds

void printArray(float* values, int length) {
  cerr << "\nplot([";
  for (int i = 0; i < length; i++) {
    if (i > 0) cerr << ',';
    cerr << values[i];
  }
  cerr << "])\n";
}

Sync::Sync(SyncConfig* cfg) : cfg(cfg) {
  //samplesPerMetaSample = (float) cfg->chipSize / cfg->detectionSamplesPerChip;

  assert(cfg->numChannels % 2 == 0);
  bitPatternAbs = sqrt((float)cfg->numChannels/2);

  //metaSamplesPerPulse = cfg->detectionSamplesPerChip * cfg->chipsPerSyncPulse;
  //cerr << metaSamplesPerPulse << "<------------\n";
  //metaSamplesPerCycle = 2 * metaSamplesPerPulse;
  //readyRepeats = 2 * cfg->chipsPerSyncPulse;
  //pulseSamples = cfg->chipsPerSyncPulse * cfg->chipSize;

  precomp = new complex<float>*[cfg->numChannels];
  for (int i = 0; i < cfg->numChannels; i++) {
    precomp[i] = new complex<float>[cfg->chipSize];
    int bucket = cfg->baseBucket + (i * cfg->channelSpacing);
    for (int j = 0; j < cfg->chipSize; j++) {
      double arg = M_PI * 2 * j * bucket / cfg->chipSize;
      precomp[i][j] = complex<float>(cos(arg), sin(arg));
    }
  }

  reset();

  //cerr << "VALS " << cfg->baseBucket << ' '
      //<< cfg->numChannels << ' ' << cfg->channelSpacing << '\n';
}

static int hack = 0;

Sync::~Sync() {
  for (int i = 0; i < cfg->numChannels; i++) {
    delete precomp[i];
  }
  delete precomp;
}

void Sync::createSyncCycles(int chipsPerPulse, int cycles, vector<float> &target) {

  int n = 0;
  for (int i = 0; i < cycles * 2 + 1; i++) {
    int pattern = i % 2;
    for (int t = 0; t < cfg->chipSize * chipsPerPulse; t++) {
      float val = 0;
      for (int chan = pattern; chan < cfg->numChannels; chan += 2) {
        val += precomp[chan][t % cfg->chipSize].imag();
      }
      n++;
      target.push_back(val);
    }
  }

  normalize(target.end() - n, target.end());
}

void Sync::generateSync(vector<float> &target) {
  createSyncCycles(cfg->chipsPerSyncPulse, cfg->longMetaBucket + 1, target);
  createSyncCycles(1, cfg->chipsPerSyncPulse * 2, target);
}

void Sync::reset() {
  //fftSampleIndex = 0;
  //syncOffset = 0;
  state = -1;
  buffer.clear();
  buffer.reserve(MAX_BUFFER_SAMPLES);
  //shortMetaBuffer.clear();
  //metaBuffer.clear();
  // TODO: reserve certain size in buffers.
}

bool Sync::receiveAudioAndSync(const vector<float> &samples,
    vector<float> &trailing) {

  buffer.insert(buffer.end(), samples.begin(), samples.end());

  int chipSize = cfg->chipSize;

  int pulseSize = cfg->chipsPerSyncPulse * chipSize;
  int pulseWindow = pulseSize;

  int signalCycleSize = 2 * pulseSize;
  int metaSignalBucket = cfg->longMetaBucket;
  int readyMetaSignalBucket = metaSignalBucket * cfg->chipsPerSyncPulse;

  int longSignalWindow = (1 + metaSignalBucket * 2) * pulseSize;
  int dataWindow = longSignalWindow + chipSize;

  int readySize = (1 + 2 * cfg->chipsPerSyncPulse) * chipSize;
  assert(readySize == signalCycleSize + chipSize);

  int metaSamplesPerPulse = cfg->chipsPerSyncPulse * cfg->detectionSamplesPerChip;
  int metaSamplesPerCycle = 2 * metaSamplesPerPulse;
  float samplesPerMetaSample = ((float)chipSize) / cfg->detectionSamplesPerChip;

  assert(cfg->chipsPerSyncPulse == (pulseWindow / chipSize));

//  if (buffer.size() < dataWindow) {
//    return false;
//  }
 // state = -1

  cerr << "Buff size " << buffer.size() << endl;

  while (buffer.size() >= dataWindow) {
    cerr << "do " << endl;
    //shortMetaSignal = []

    //for i in xrange(longSignalWindow * self.detectionSamplesPerChip / chipSize):
    //  start = int(i * samplesPerMetaSample)
    //  end = start + chipSize # + pulseWindow
    //  assert end <= len(data)
    //  spectrum = np.fft.rfft(data[start:end])
    //  bucketVals = [spectrum[b] for b in bucketIndices]
    //  match = self.detectStrategy(bucketVals, pattern)
    //  shortMetaSignal.append(match)

    vector<float> shortMetaSignal;
    assert(shortMetaSignal.size() == 0);

    for (int i = 0; i < longSignalWindow * cfg->detectionSamplesPerChip / chipSize; i++) {
     //// Direct version, faster for numChannels less than about 24ish
      int start = (int)(i * samplesPerMetaSample);
      int end = start + chipSize;
      assert(end <= buffer.size());
      
      int numChans = cfg->numChannels;
      complex<float> bucketVals[numChans];
      //assert (buffer.size() > bufferStart() + cfg->chipSize);
      for (int c = 0; c < numChans; c++) {
        bucketVals[c] = 0;
        float *b, *bEnd = buffer.data() + end;
        complex<float> *p, *pEnd = precomp[c] + cfg->chipSize;
        for (b = buffer.data() + start, p = precomp[c];
            b < bEnd && p < pEnd; b++, p++) {
          //bucketVals[c] += buffer[i] * precomp[c][i - bufferStart()];
          bucketVals[c] += (*b) * (*p);
        }
      }
      float match = detectMatch(bucketVals);
      shortMetaSignal.push_back(match);
    }

   //// (FFT version)
   // Spectrum spectrum(buffer.data() + bufferStart(), SAMPLE_RATE, cfg->chipSize);
   // complex<float> bucketVals[cfg->numChannels];
   // //copyBucketVals(spectrum, cfg->chipsPerSyncPulse, bucketVals);
   // copyBucketVals(spectrum, 1, bucketVals);

   // float match = detectMatch(bucketVals);
   // shortMetaBuffer.push_back(match);
   // fftSampleIndex++;

    vector<float> longMetaSignal;
    assert(longMetaSignal.size() == 0);

    for (int i = 0; i < metaSamplesPerCycle * metaSignalBucket; i++) {
      float tot = 0;
      for (int j = i; j < i+metaSamplesPerPulse; j++) {
        tot += shortMetaSignal[j];
      }

      float lms = tot / metaSamplesPerPulse;
      assert(lms >= -1 && lms <= 1);
      longMetaSignal.push_back(lms);
    }
    assert(longMetaSignal.size() == metaSamplesPerCycle * metaSignalBucket);

    cerr << endl;
    //printArray(shortMetaSignal.data(), shortMetaSignal.size());
    //printArray(longMetaSignal.data(), longMetaSignal.size());

    //longMetaSpectrum = np.fft.rfft(longMetaSignal)
    //partALength = metaSamplesPerCycle * (metaSignalBucket - 1)
    //assert partALength >= metaSamplesPerCycle
    //longMetaSpectrumA = np.fft.rfft(longMetaSignal[:partALength])
    //longMetaSpectrumB = np.fft.rfft(longMetaSignal[partALength:])

    //partBLength = self.chipsPerSyncPulse * (self.detectionSamplesPerChip * 2)
    //#shortMetaSpectrumA = np.fft.rfft(shortMetaSignal[:-partBLength])
    //shortMetaSpectrumB = np.fft.rfft(shortMetaSignal[-partBLength:])

    Spectrum longMetaSpectrum(longMetaSignal.data(), -1, longMetaSignal.size());
    int partALength = metaSamplesPerCycle * (metaSignalBucket - 1);
    assert(partALength >= metaSamplesPerCycle);
    Spectrum longMetaSpectrumA(longMetaSignal.data(),-1,  partALength);
    Spectrum longMetaSpectrumB(longMetaSignal.data() + partALength, -1,
        longMetaSignal.size() - partALength);

    int partBLength = cfg->chipsPerSyncPulse * (cfg->detectionSamplesPerChip * 2);
    Spectrum shortMetaSpectrumB(
        shortMetaSignal.data() + shortMetaSignal.size() - partBLength, -1,
        partBLength);

    float misalignment = 0;

    if (state == 2) {
      float chipMisalignment;
      int resultShortB = getAlignment(shortMetaSpectrumB,
          cfg->chipsPerSyncPulse, 1, 1, &chipMisalignment);
      if (resultShortB == 1) {
        ll(LOG_INFO, "SCOM", "SUCCESS %f", chipMisalignment);
        assert(buffer.begin() + dataWindow < buffer.end());
        trailing.insert(trailing.end(), buffer.begin() + dataWindow, buffer.end());
        cerr << ">>>>>>>>   Trailing size " << trailing.size() << endl;
        reset();
        return true;
      } else {
        state = -1;
        cerr << "Missed it " << endl;
      }
    }

    if (state == 1) {
      int resultLongA = getAlignment(longMetaSpectrumA,
          metaSignalBucket - 1, 1, cfg->chipsPerSyncPulse, NULL);
      int resultLongB = getAlignment(longMetaSpectrumB,
          1, 1, cfg->chipsPerSyncPulse, NULL);
      float chipMisalignment;
      int resultShortB = getAlignment(shortMetaSpectrumB,
          cfg->chipsPerSyncPulse, 1, 1, &chipMisalignment);
      //('\n|| %s %s %s' % (resultLongA, resultLongB, resultShortB))
      if (resultLongA == 1 && resultLongB < 1 && resultShortB >= 0) {
        state = 2;
        misalignment = chipMisalignment / cfg->chipsPerSyncPulse;
      }
    }

    if (state < 2) {
      state = getAlignment(longMetaSpectrum,
          metaSignalBucket, state, cfg->chipsPerSyncPulse, &misalignment);
    }

    if (state < 0) {
      misalignment = 0;
    }
    cerr << "misaligned " << misalignment << endl;

    int alignedAmount = (int)(signalCycleSize * (1 - misalignment));

    buffer.erase(buffer.begin(), buffer.begin() + alignedAmount); 
    //data = data.er[alignedAmount:] + list(receiver.receiveBlock(alignedAmount))

    cerr << "state " << state << "  aligned " << alignedAmount << endl;
  }

  return false;
}

/*
bool Sync::receiveAudioAndSync(const vector<float> &samples,
    vector<float> &trailing) {
  hack++;

  if (state == -1 && samples.size() > MAX_BUFFER_SAMPLES) {
    assert(false);
    // TODO: use fixed size float arrays, no need for growing vectors
    reset();
  }

  //cerr << "do ";
  //cerr << "\n\n\n===========\nCALLED with " << samples.size() << " samples\n";
  // Append samples to buffer
  // TODO: just use a rotating buffer of a fixed float array
  buffer.insert(buffer.end(), samples.begin(), samples.end());

  // need extra chipSize at the end for possible alignment signal,
  // which is 1 long cycle + 1 chip in size (sync signals need to
  // have an odd number of pulses).

  while (buffer.size() >= bufferStart() + pulseSamples + cfg->chipSize * 10) {
   //// Direct version, faster for numChannels less than about 24ish
    int numChans = cfg->numChannels;
    complex<float> bucketVals[numChans];
    assert (buffer.size() > bufferStart() + cfg->chipSize);
    for (int c = 0; c < numChans; c++) {
      bucketVals[c] = 0;
      float *b, *bEnd = buffer.data() + bufferStart() + cfg->chipSize;
      complex<float> *p, *pEnd = precomp[c] + cfg->chipSize;
      for (b = buffer.data() + bufferStart(), p = precomp[c];
          b < bEnd && p < pEnd; b++, p++) {
        //bucketVals[c] += buffer[i] * precomp[c][i - bufferStart()];
        bucketVals[c] += (*b) * (*p);
      }
    }
    float match = detectMatch(bucketVals);
    shortMetaBuffer.push_back(match);
    fftSampleIndex++;

   //// (FFT version)
   // Spectrum spectrum(buffer.data() + bufferStart(), SAMPLE_RATE, cfg->chipSize);
   // complex<float> bucketVals[cfg->numChannels];
   // //copyBucketVals(spectrum, cfg->chipsPerSyncPulse, bucketVals);
   // copyBucketVals(spectrum, 1, bucketVals);

   // float match = detectMatch(bucketVals);
   // shortMetaBuffer.push_back(match);
   // fftSampleIndex++;

    if (fftSampleIndex >= cfg->detectionSamplesPerChip) {
      //metaBuffer.push_back(match);
      float tot = 0;
      for (int j = fftSampleIndex - cfg->detectionSamplesPerChip; j < fftSampleIndex; j++) {
        tot += shortMetaBuffer[j];
      }
      metaBuffer.push_back(tot / cfg->detectionSamplesPerChip);
    }
  }

      //return samples.end();

  //cerr << "\nsyncOffset " << syncOffset << ' ' << 2*pulseSamples << '\n';
  int longMetaSpectrumLength = metaSamplesPerCycle * cfg->longMetaBucket;

  while (true) {
    float metaBufferStartFloat = syncOffset / samplesPerMetaSample;
    int metaBufferStart = (int) metaBufferStartFloat;
    float bufferMisalignment = (metaBufferStart - metaBufferStartFloat)
        / cfg->detectionSamplesPerChip;
    float cock = bufferMisalignment;
    bufferMisalignment = 0;
    assert(bufferMisalignment <= 0);
    assert(abs(bufferMisalignment) < 1.0 / cfg->detectionSamplesPerChip);

    //cerr << "\n---------loop " << syncOffset << ", " << metaBufferStart << '\n';
    //assert(metaBufferStart < metaBuffer.size());

    if (metaBuffer.size() < metaBufferStart + longMetaSpectrumLength) {
      // No sync yet
      return false;// samples.end();
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
      //cerr << "X ";

      int readySignalStart = 
        syncOffset + ((cfg->longMetaBucket - 1) * 2 + 1) * pulseSamples;
      //cerr << "SYNC OFFSET " << syncOffset << ' ' << readySignalStart << '\n';
      int shortMetaSpectrumLength = metaSamplesPerCycle;
      float readyBuffer[shortMetaSpectrumLength * 2];//XXX
      for (int i = 0; i < shortMetaSpectrumLength * 2; i++) {
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

      if (state == 2) {
        cerr << "READY BUFFER" << endl;
        printArray(readyBuffer, shortMetaSpectrumLength*2);
        float chipMisalignment;
        int resultShortB = getAlignment(shortMetaSpectrumB, cfg->chipsPerSyncPulse,
            1, 1, bufferMisalignment, &chipMisalignment);
        if (resultShortB == 1) {
          //logging.debug('state 2 ALIGNED')
          // XXX return actual alignment
          //int offset = readySignalStart + (int)(samplesPerMetaSample*shortMetaSpectrumLength + (1-chipMisalignment)*cfg->chipSize);
          //int offset = readySignalStart + (int)(samplesPerMetaSample*shortMetaSpectrumLength + cfg->chipSize  * (1 - misalignment + cock));
          int offset = readySignalStart + (int)(samplesPerMetaSample*shortMetaSpectrumLength + cfg->chipSize);

          cerr << "!!!!!!!!! " << offset << ' ' << cfg->chipSize << endl;
          offset += cfg->chipSize*2; //XXX XXX XXX
          cerr << "!!!!!!!!! " << offset << ' ' << cfg->chipSize << endl;
          cerr << "COCK " << cock << ' ' << misalignment << endl;
          //int inputOffset = samples.size() - (buffer.size() - offset);
          //if (inputOffset < 0) {
          //  ll(LOG_WARN, "SCOM", 
          //      "input offset before sample start, probably bad sync %d %f %f",
          //      inputOffset, bufferMisalignment, chipMisalignment);
          //  reset();
          //  return samples.end();
          //}
          //if (inputOffset > samples.size()) {
          //  ll(LOG_WARN, "SCOM", "input offset after sample end, probably bad sync");
          //  reset();
          //  return false;
          //}
          if (offset < 0) {
            ll(LOG_WARN, "SCOM", 
                "input offset before sample start, probably bad sync %d %f %f",
                offset, bufferMisalignment, chipMisalignment);
            reset();
            return false;
          }
          if (offset > buffer.size()) {
            ll(LOG_WARN, "SCOM", "input offset after sample end, probably bad sync");
            reset();
            assert(false);
            return false;
          }
          ll(LOG_INFO, "SCOM", "SUCCESS %d %f",
              offset, chipMisalignment);
          assert(trailing.size() == 0);
          trailing.insert(trailing.end(), buffer.begin() + offset, buffer.end());
          cerr << "TRAILING";
          //printArray(trailing.data(), trailing.size());

          reset();
          cerr << "========================= HACK " << hack << endl;
    //if (hack == 63) assert(false);
          return true;
        } else {
          ll(LOG_INFO, "SCOM", "Argh missed it %d %f", resultShortB, chipMisalignment);
          state = -1;
        }
    //if (hack == 63) assert(false);
    //assert(false);
      }
      
      if (state == 1) {
        int resultLongA = getAlignment(longMetaSpectrumA,
            cfg->longMetaBucket - 1, 1, cfg->chipsPerSyncPulse, bufferMisalignment, NULL);
        int resultLongB = getAlignment(longMetaSpectrumB,
            1, 1, cfg->chipsPerSyncPulse, bufferMisalignment, NULL);
        float chipMisalignment;
        int resultShortB = getAlignment(shortMetaSpectrumB,
            cfg->chipsPerSyncPulse, 1, 1, bufferMisalignment, &chipMisalignment);
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

    //if (hack == 61) {
    //  cerr << "META BUFFER " << endl;
    //  printArray(longMetaSignal, longMetaSpectrumLength);
    //  cerr << "NEW STATE IS " << state <<  endl;
    //}

    }

    if (state < 2) {
      // Check for, and align to, the long signal
      //#logging.debug('L')
      state = getAlignment(longMetaSpectrum,
          cfg->longMetaBucket, state, cfg->chipsPerSyncPulse, 
          bufferMisalignment, &misalignment);
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

    cock /= cfg->chipsPerSyncPulse;
    cerr << "misalignment " << misalignment << "  known " << cock << endl;
    cock = 0;
    int alignedAmount = (int)(2*pulseSamples * (1 - misalignment + cock));

    syncOffset += alignedAmount;

    cerr << "state " << state << endl;

  }
}
*/
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
    /*float knownMisalignmentPerChip,*/ float *misalignmentOut) {
  //assert(knownMisalignmentPerChip == 0);
  //char label = numChips == 1 ? '!' : '.'; // for debugging

  // TODO: make misalignment ALWAYS be per chip in this code and
  // its return values.
  //float knownMisalignment = knownMisalignmentPerChip / numChips;
  
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
    cerr << misalignment << " <------------ IT " << endl;
    assert(misalignment >= -0.5 && misalignment <= 0.5);
    //misalignment -= knownMisalignment;
    //if (misalignment < -0.5) misalignment += 1.0;
    //if (misalignment > +0.5) misalignment -= 1.0;
    float misalignmentPerChip = misalignment * numChips;
    if (state == -1) {
      //logging.debug('[%s]' % label)
      result = 0; // # don't consider misalignment on first go
    //} else if (abs(misalignmentPerChip - knownMisalignmentPerChip) <= cfg->misalignmentTolerance) {
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

  //cerr << "Mis " << misalignment << " known " << knownMisalignment << endl;
  //if (misalignmentOut) *misalignmentOut = misalignment - knownMisalignment;
  if (misalignmentOut) *misalignmentOut = misalignment;// - knownMisalignment;
  return result;
}


//int Sync::bufferStart() {
//  return (int) (samplesPerMetaSample * fftSampleIndex);
//}


const float HALF_PI = 1.57079632679f;
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
  result *= 0.999;
  result += 0.0005;

  result = result * 2 - 1;
  return result;

  // Convert to linear correlation
  result = HALF_PI - 2*acos(result);

  // Map to range -1,1, with sinewave correlation
  //result = sin(result);
  result = result / 1.57079632679f;

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
