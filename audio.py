#!/usr/bin/env python

import logging
import math
import numpy as np
import pyaudio
import socket
import struct
import time

# Consts
SAMPLE_RATE = 44100
CHANNELS = 1
FORMAT = pyaudio.paInt16
PCM_QUANT = 32767.5

# Returns nsamples samples of zero
def silence(nsamples):
  samples = [0] * nsamples
  return np.array(samples)

# Returns a sine wave at freq Hz for nsamples samples, varying between +/-1.0
def sinewave(freq, nsamples):
  twoPiFOnRate = 2 * math.pi * freq / float(SAMPLE_RATE)
  samples = []
  for i in xrange(nsamples):
      samples.append(math.sin(i * twoPiFOnRate))
  return np.array(samples)

def sinewaves(freqs, nsamples):
  #begin = time.time()
  # Direct sum of sinewaves
  #waveforms = [ sinewave(f, nsamples) for f in freqs ]
  #waveform = combine(waveforms)
  #print time.time() - begin
  #return waveform

  # Or, inverse FFT, ~10x faster
  # A zero-frequency bucket, then nsamples/2 frequency buckets
  buckets = [0] * (nsamples / 2 + 1)
  for f in freqs:
    bucket = f / float(SAMPLE_RATE) * nsamples
    #assert bucket == int(bucket)
    buckets[int(bucket)] = 1.0
  waveform = np.fft.irfft(buckets, nsamples)
  # Normalize
  waveform = waveform / max(waveform)
  #print time.time() - begin
  return waveform

# Sums and normalizes a collection of waveforms to +/- 1.0
def combine(waveforms):
  agg = sum(waveforms)
  # TODO: implement dynamic range compression instead
  agg = normalise(agg)
  return agg

def normalise(waveform):
  return waveform / max(abs(waveform))

def limit(waveform):
  for i in xrange(len(waveform)):
    waveform[i] = min(waveform[i], 1.0)
    waveform[i] = max(waveform[i], -1.0)

# Compute a signal with energy at each frequency corresponding to a
# non-zero number in chip
def buildWaveform(chip, base, spacing, samples):
  freqs = [ base + spacing * i for i in xrange(len(chip)) if chip[i] > 0 ]
  return sinewaves(freqs, samples)

# Envelopes a waveform by a window function
def window(waveform):
    return np.hanning(len(waveform)) * waveform

# Fades a waveform in-place
def fadein(waveform, samples):
  n = min(len(waveform), samples)
  for i in xrange(samples):
    waveform[i] *= (float(i) / samples)

# Fades a waveform in-place
def fadeout(waveform, samples):
  n = min(len(waveform), samples)
  for i in xrange(len(waveform) - n, len(waveform)):
    waveform[i] *= (len(waveform) - i) / float(n)

# Encodes waveform amplitudes as a little-endian PCM-16 stream
def encodePcm(waveform):
  shorts = [math.floor(PCM_QUANT * f) for f in waveform]
  return struct.pack("<%dh" % len(shorts), *shorts)

# Decodes a stream of little-endian PCM-16 into waveform amplitudes
def decodePcm(block):
  shorts = struct.unpack("<%dh" % (len(block) / 2), block)
  return (np.array(shorts) + 0.5) / PCM_QUANT

# Computes DFT over an array of time-domain samples
# BucketWidth is the 
def fouriate(samples):
  # rfft computes FFT of real-valued inputs, producing n/2+1 amplitude values.
  # buckets[0] is the zero-frequency term (i.e. signal mean/DC offset)
  buckets = np.fft.rfft(np.array(samples))
  bucketWidth = SAMPLE_RATE / len(samples)
  return Spectrum(buckets, bucketWidth)


class Spectrum(object):
  def __init__(self, data, bucketWidth):
    assert bucketWidth > 0
    self.data = data
    self.bucketWidth = bucketWidth

  # The amplitude of the signal around freq
  def amplitude(self, freq):
    return abs(self.data[self.getIndex(freq)])

  # The power of the signal around freq
  def power(self, freq):
    return self.amplitude(freq)**2

  # The complex-valued FFT value around freq
  def getIndex(self, freq):
    assert freq % self.bucketWidth == 0, '%fHz is not a bucket centre' % freq
    index = freq / self.bucketWidth
    return index

def createAudioStream(pa, isInput):
  return pa.open(
      rate = SAMPLE_RATE,
      channels = CHANNELS,
      input = isInput,
      output = (not isInput),
      format = FORMAT)


# Implied:
#
# interface Receiver {
#   double[] receiveBlock(int numSamples)
# }
#
# interface Sender {
#   void sendBlock(double[] waveForm)
# }
#

# Receiver which reads from PyAudio's default input device
class PyAudioReceiver(object):
  def __init__(self):
    self.stream = createAudioStream(pyaudio.PyAudio(), isInput=True)

  def receiveBlock(self, numSamples):
    block = None
    while block is None:
      try:
        #print self.stream.get_read_available(), "frames available"
        block = self.stream.read(numSamples)
        #print "Read", numSamples, "samples"
      except IOError as ex:
        if ex[1] != pyaudio.paInputOverflowed:
          raise
        logging.info("Dropped audio frame attempting to read %d samples! Input overflowed." % numSamples)

    assert len(block) / 2 == numSamples
    return decodePcm(block)

# Receiver which reads from an input stream
class StreamReceiver(object):
  def __init__(self, stream):
    self.stream = stream

  def receiveBlock(self, numSamples):
    nBytes = numSamples * 2
    block = ""
    while len(block) < nBytes:
      block = block + self.stream.read(nBytes - len(block))
      if len(block) == 0:
        assert False
    return decodePcm(block)


class PyAudioSender(object):
  def __init__(self):
    self.stream = createAudioStream(
      pyaudio.PyAudio(), isInput=False)

  def sendBlock(self, waveform):
    self.stream.write(encodePcm(waveform))

# Sender which writes to an output stream
class StreamSender(object):
  def __init__(self, stream):
    self.stream = stream

  def sendBlock(self, waveform):
    self.stream.write(encodePcm(waveform))
    self.stream.flush()
