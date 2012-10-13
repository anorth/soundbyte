#!/usr/bin/env python

import math
import numpy as np
import pyaudio
import struct

pa = pyaudio.PyAudio()

# CONSTS
RATE = 44100
CHANNELS = 1
FORMAT = pyaudio.paInt16


#PARAMS
INPUT_BLOCK_TIME = 0.02
SYMBOLS_SEC = 10
CHUNK = int(RATE / SYMBOLS_SEC)
F_BASE = 18000
F_SPACE = 200




def silence(count):
  samples = [0] * count
  return np.array(samples)

def sinewave(freq, count):
  frate = float(RATE)
  samples = []
  for i in xrange(count):
      samples.append(math.sin(2*math.pi*freq*(i/frate)))
  return np.array(samples)

def combine(waveforms):
    agg = sum(waveforms)
    # TODO: implement dynamic range compression instead
    agg = agg / max(agg)

def encode(floats, smoothEdges=True):
  if smoothEdges:
    floats = np.hanning(len(floats)) * floats
  shorts = [32767 * f for f in floats]
  return struct.pack("%dh" % len(shorts), *shorts)

def fouriate(shorts, timeLengthSeconds):
  return Spectrum(np.fft.rfft(np.array(shorts)), timeLengthSeconds)

class Spectrum(object):
  def __init__(self, data, timeLengthSeconds):
    self.data = data
    self.timeLengthSeconds = timeLengthSeconds

  def intensity(self, freq):
    return abs(self.data[self.getIndex(freq)])

  def getIndex(self, freq):
    index = freq * self.timeLengthSeconds
    intIndex = int(index)
    assert index == intIndex, 'suboptimal frequency choice %d' % freq
    return intIndex


class AudioHelper(object):
  def __init__(self):
    self.pa = pyaudio.PyAudio()

  def openStreamInner(self, isInput):
    # TODO: support file inputs.
    self.stream = self.pa.open(
        rate = RATE,
        channels = CHANNELS,
        input = isInput,
        output = (not isInput),
        format = FORMAT)

  def close(self):
    self.stream.close()

class AudioReceiver(AudioHelper):
  def __init__(self):
    pass

  def openStream(self):
    self.openStreamInner(isInput=True)

  def receiveBlock(self, timePeriodSeconds):
    numFrames = timePeriodSeconds * RATE
    #try:
    #  block = self.stream.read(numFrames)
    #except IOError, e:
    #  # dammit. 
    #  print( "(%d) Error recording: %s"%(self.errorcount,e) )
    #  return
    block = self.stream.read(numFrames)
    count = len(block) / 2
    shorts = struct.unpack("%dh" % count, block)

    return shorts


class AudioSender(AudioHelper):
  def __init__(self):
    pass

  def openStream(self):
    self.openStreamInner(isInput=False)

  def sendWaveForm(floats):
    stream.write(encode(floats))
