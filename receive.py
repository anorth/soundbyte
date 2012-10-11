#!/usr/bin/python

# open a microphone in pyAudio and listen for taps

import sys
import pyaudio
import struct
import math
import wave
import numpy as np
import time
from matplotlib.pyplot import plot

INITIAL_TAP_THRESHOLD = 0.010
FORMAT = pyaudio.paInt16 
SHORT_NORMALIZE = (1.0/32768.0)
CHANNELS = 2
RATE = 44100  
INPUT_BLOCK_TIME = 0.02
INPUT_FRAMES_PER_BLOCK = int(RATE*INPUT_BLOCK_TIME)
# if we get this many noisy blocks in a row, increase the threshold
OVERSENSITIVE = 15.0/INPUT_BLOCK_TIME          
# if we get this many quiet blocks in a row, decrease the threshold
UNDERSENSITIVE = 120.0/INPUT_BLOCK_TIME 
# if the noise was longer than this many blocks, it's not a 'tap'
MAX_TAP_BLOCKS = 0.15/INPUT_BLOCK_TIME

WINDOW_TIME = 0.05

def get_rms( block ):
  # RMS amplitude is defined as the square root of the 
  # mean over time of the square of the amplitude.
  # so we need to convert this string of bytes into 
  # a string of 16-bit samples...

  # we will get one short out for each 
  # two chars in the string.
  count = len(block)/2
  format = "%dh"%(count)
  shorts = struct.unpack( format, block )

  # iterate over the block.
  sum_squares = 0.0
  for sample in shorts:
    # sample is a signed short in +/- 32768. 
    # normalize it to 1.0
    n = sample * SHORT_NORMALIZE
    sum_squares += n*n

  return math.sqrt( sum_squares / count )

class TapTester(object):
  def __init__(self):
    self.pa = pyaudio.PyAudio()
    self.stream = self.open_mic_stream()
    self.tap_threshold = INITIAL_TAP_THRESHOLD
    self.noisycount = MAX_TAP_BLOCKS+1 
    self.quietcount = 0 
    self.errorcount = 0

    self.thresh = 20
    self.winFactor = 0.6
    self.heartWin = int(WINDOW_TIME / INPUT_BLOCK_TIME)
    self.win = int(self.winFactor * WINDOW_TIME / INPUT_BLOCK_TIME)
    self.numchans = 7
    self.bitsignals = [[0] * self.win] * self.numchans
    self.heartSignals = [0] * self.heartWin

    self.lastHeart = 0
    self.heartVal = 0

  def stop(self):
    self.stream.close()

  def find_input_device(self):
    device_index = None      
    for i in range( self.pa.get_device_count() ):   
      devinfo = self.pa.get_device_info_by_index(i)   
      print( "Device %d: %s"%(i,devinfo["name"]) )

      for keyword in ["mic","input"]:
        if keyword in devinfo["name"].lower():
          print( "Found an input: device %d - %s"%(i,devinfo["name"]) )
          device_index = i
          return device_index

    if device_index == None:
      print( "No preferred input found; using default input device." )

    return device_index

  def open_mic_stream( self ):
    device_index = self.find_input_device()

    stream = self.pa.open(   format = FORMAT,
                 channels = CHANNELS,
                 rate = RATE,
                 input = True,
                 input_device_index = device_index,
                 frames_per_buffer = INPUT_FRAMES_PER_BLOCK)

    return stream

  def tapDetected(self):
    print "Tap!"

  def listen(self):
    try:
      block = self.stream.read(INPUT_FRAMES_PER_BLOCK)
    except IOError, e:
      # dammit. 
      self.errorcount += 1
      print( "(%d) Error recording: %s"%(self.errorcount,e) )
      self.noisycount = 1
      return

    count = len(block)/2
    format = "%dh"%(count)
    shorts = struct.unpack( format, block )
#
#    # get rms
#    sum_squares = 0.0
#    for sample in shorts:
#      # sample is a signed short in +/- 32768. 
#      # normalize it to 1.0
#      n = sample * SHORT_NORMALIZE
#      sum_squares += n*n
#
#    rms = math.sqrt( sum_squares / count )

    window = np.blackman(count)
    data = np.array(shorts) #* window

    x = np.fft.rfft(data)

    #print a, [abs(x[o]) for o in others] 

    def freqVal(freq):
      #print int(freq * INPUT_BLOCK_TIME),
      return abs(x[int(freq * INPUT_BLOCK_TIME)])

    base = 16500
    chansibling = 200
    chandiff = 2 * chansibling
    assert chansibling * INPUT_BLOCK_TIME >= 2
    for i in range(0, self.numchans):
      ia = freqVal(base + i * chandiff)
      ib = freqVal(base + i * chandiff + chansibling)
      self.bitsignals[i] = self.bitsignals[i][1:] + [ia > ib and 1 or 0]

    heartA = freqVal(base - chandiff)
    heartB = freqVal(base - chandiff + chansibling)

    factor = 1.0

    inc = 3

    mx = self.thresh * inc
    dt = mx

    self.heartVal = min(mx, max(-mx, self.heartVal))

    if (heartA > heartB * factor):
      self.heartSignals = self.heartSignals[1:] + [1]
      #self.heartVal += inc
      #print '_',
    elif (heartB > heartA * factor):
      self.heartSignals = self.heartSignals[1:] + [-1]
      #self.heartVal -= inc
      #print 'X',
    else:
      self.heartSignals = self.heartSignals[1:] + [0]
      #self.heartVal -= np.sign(self.heartVal)
      #print ' ',
      pass

    gotBeat = False
    heartSum = sum(self.heartSignals)
    if abs(heartSum) >= self.winFactor * self.heartWin:
      h = np.sign(heartSum)
      if h != self.lastHeart:
        gotBeat = True
        self.lastHeart = np.sign(heartSum)

    #print self.heartVal,

    #if self.lastHeart == 1:
    #  self.heartVal = max(0, self.heartVal)
    #if self.lastHeart == -1:
    #  self.heartVal = min(0, self.heartVal)

    #gotBeat = False
    #
    #if self.heartVal == dt and self.lastHeart != 1:
    #  print 'A>>',
    #  self.lastHeart = 1
    #  gotBeat = True

    #if self.heartVal == -dt and self.lastHeart != -1:
    #  print 'B>>',
    #  self.lastHeart = -1
    #  gotBeat = True

    if gotBeat:
      v = 0
      pos = 1
      #print '=============================='
      for s in self.bitsignals:
        if np.mean(s) >= 0.5:
          #print 1,
          v += pos
        else:
          #print 0,
          pass
        pos *= 2
      #print

      if v != 0: 
        if v == 13: print
        #print '==============================', chr(v), '=======================\n'
        else: sys.stdout.write(chr(v))


    sys.stdout.flush()



    #print x, len(x), count
    #plot(x)
    #print "THERE"
    #time.sleep(2)

  def otherlisten(self):
    try:
      block = self.stream.read(INPUT_FRAMES_PER_BLOCK)
    except IOError, e:
      # dammit. 
      self.errorcount += 1
      print( "(%d) Error recording: %s"%(self.errorcount,e) )
      self.noisycount = 1
      return

    amplitude = get_rms( block )
    if amplitude > self.tap_threshold:
      # noisy block
      self.quietcount = 0
      self.noisycount += 1
      if self.noisycount > OVERSENSITIVE:
        # turn down the sensitivity
        self.tap_threshold *= 1.1
    else:      
      # quiet block.

      if 1 <= self.noisycount <= MAX_TAP_BLOCKS:
        self.tapDetected()
      self.noisycount = 0
      self.quietcount += 1
      if self.quietcount > UNDERSENSITIVE:
        # turn up the sensitivity
        self.tap_threshold *= 0.9

if __name__ == "__main__":
  tt = TapTester()

  print "Listening..."
  while True:
    tt.listen()
