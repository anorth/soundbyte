#!/usr/bin/env python

import math
import numpy as np
import pyaudio
import socket
import struct

# Consts
SAMPLE_RATE = 44100
CHANNELS = 1
FORMAT = pyaudio.paInt16
PCM_QUANT = 32767.5

# Returns count samples of zero
def silence(count):
  samples = [0] * count
  return np.array(samples)

# Returns a sine wave at freq Hz for count samples, varying between +/-1.0
def sinewave(freq, count):
  twoPiFOnRate = 2 * math.pi * freq / float(SAMPLE_RATE)
  samples = []
  for i in xrange(count):
      samples.append(math.sin(i * twoPiFOnRate))
  return np.array(samples)

# Sums and normalizes a collection of waveforms to +/- 1.0
def combine(waveforms):
  agg = sum(waveforms)
  # Normalise
  # TODO: implement dynamic range compression instead
  agg = agg / max(agg)
  return agg

# Envelopes a waveform by a window function
def window(waveform):
    return np.hanning(len(waveform)) * waveform

# Encodes waveform amplitudes as a little-endian PCM-16 stream
def encode(waveform):
  shorts = [math.floor(PCM_QUANT * f) for f in waveform]
  return struct.pack("<%dh" % len(shorts), *shorts)

# Decodes a stream of little-endian PCM-16 into waveform amplitudes
def decode(block):
  shorts = struct.unpack("<%dh" % (len(block) / 2), block)
  return window((np.array(shorts) + 0.5) / PCM_QUANT)

# Computes DFT over an array of time-domain samples
# BucketWidth is the 
def fouriate(samples):
  # rfft computes FFT of real-valued inputs, producing n/2+1 amplitude values.
  # buckets[0] is the zero-frequency term (i.e. signal mean/DC offset)
  buckets = np.fft.rfft(np.array(samples))
  bucketWidth = SAMPLE_RATE / len(samples)
  return Spectrum(buckets, bucketWidth)

# Computes the decibel ratio between two power quantities
def dbPower(a, b):
  return 10 * math.log10(a / b)

# Computes the decibel ratio between two amplitude quantities
def dbAmplitude(a, b):
  return 2 * dbPower(a, b)
  # == 10 * log10(a**2 / b**2)


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
#   short[] receiveBlock(int numSamples)
# }
#
# interface Sender {
#   void sendWaveForm(double[] waveForm)
# }
#

# Receiver which reads from PyAudio's default input device
class PyAudioReceiver(object):
  def __init__(self):
    self.stream = createAudioStream(pyaudio.PyAudio(), isInput=True)

  def receiveBlock(self, numSamples):
    #try:
    #  block = self.stream.read(Samples)
    #except IOError, e:
    #  # dammit. 
    #  print( "(%d) Error recording: %s"%(self.errorcount,e) )
    #  return
    block = self.stream.read(numSamples)
    assert len(block) / 2 == numSamples
    return decode(block)

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
    return decode(shorts)


class PyAudioSender(object):
  def __init__(self):
    self.stream = createAudioStream(
      pyaudio.PyAudio(), isInput=False)

  def sendWaveForm(self, waveform):
    #waveform = window(waveform)
    self.stream.write(encode(waveform))
