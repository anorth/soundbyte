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


def silence(count):
  samples = [0] * count
  return np.array(samples)

def sinewave(freq, count):
  twoPiFOnRate = 2 * math.pi * freq / float(SAMPLE_RATE)
  samples = []
  for i in xrange(count):
      samples.append(math.sin(i * twoPiFOnRate))
  return np.array(samples)

def combine(waveforms):
  agg = sum(waveforms)
  # Normalise
  # TODO: implement dynamic range compression instead
  agg = agg / max(agg)
  return agg

def encode(floats, smoothEdges=True):
  if smoothEdges:
    floats = np.hanning(len(floats)) * floats
  shorts = [32767 * f for f in floats]
  return struct.pack("%dh" % len(shorts), *shorts)

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
#   short[] receiveBlock(int numSamples)
# }
#
# interface Sender {
#   void sendWaveForm(double[] waveForm)
# }
#

# TODO: implement StdinReceiver, etc.
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
    count = len(block) / 2
    assert count == numSamples
    shorts = struct.unpack("%dh" % count, block)

    return shorts

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

    count = len(block) / 2
    # Shorts are expected little-endian
    shorts = struct.unpack("<%dh" % count, block)
    return shorts


class PyAudioSender(object):
  def __init__(self):
    self.stream = createAudioStream(
      pyaudio.PyAudio(), isInput=False)

  def sendWaveForm(self, floats):
    self.stream.write(encode(floats))
