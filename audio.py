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
  frate = float(SAMPLE_RATE)
  samples = []
  for i in xrange(count):
      samples.append(math.sin(2*math.pi*freq*(i/frate)))
  return np.array(samples)

def combine(waveforms):
    agg = sum(waveforms)
    # TODO: implement dynamic range compression instead
    agg = agg / max(agg)
    return agg

def encode(floats, smoothEdges=True):
  if smoothEdges:
    floats = np.hanning(len(floats)) * floats
  shorts = [32767 * f for f in floats]
  return struct.pack("%dh" % len(shorts), *shorts)

def fouriate(shorts, timeLengthSeconds):
  return Spectrum(np.fft.rfft(np.array(shorts)), timeLengthSeconds)

class Spectrum(object):
  def __init__(self, data, timeLengthSeconds):
    assert timeLengthSeconds > 0
    self.data = data
    self.timeLengthSeconds = timeLengthSeconds

  def intensity(self, freq):
    return abs(self.data[self.getIndex(freq)])

  def getIndex(self, freq):
    index = freq * self.timeLengthSeconds
    intIndex = int(index)
    assert index == intIndex, 'suboptimal frequency choice %d' % freq
    return intIndex

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
    print self.stream

  def receiveBlock(self, numSamples):
    nBytes = numSamples * 2
    block = ""
    while len(block) < nBytes:
      block = block + self.stream.read(nBytes - len(block))
      #print "read", len(block), "bytes"
      if len(block) == 0:
        assert False

    count = len(block) / 2
    assert count == numSamples
    return struct.unpack(">%dh" % count, block)


class PyAudioSender(object):
  def __init__(self):
    self.stream = createAudioStream(
      pyaudio.PyAudio(), isInput=False)

  def sendWaveForm(self, floats):
    self.stream.write(encode(floats))
