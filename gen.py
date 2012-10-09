#!/usr/bin/env python

import math
import numpy
import pyaudio
from struct import pack
import sys
import termios
import tty

pa = pyaudio.PyAudio()

RATE = 44100
CHANNELS = 1
FORMAT = pyaudio.paInt16
SYMBOLS_SEC = 20
CHUNK = int(RATE / SYMBOLS_SEC)

F_BASE = 16500
F_SPACE = 200
SIGNAL = "abcd"

def openStream():
  stream = pa.open(rate = RATE, 
      channels = CHANNELS, 
      output = True,
      format = FORMAT)
  return stream

def silence(cnt):
  samples = [0] * cnt
  return numpy.array(samples)

def sinewave(freq, cnt):
  frate = float(RATE)
  samples = []
  for i in xrange(cnt):
      samples.append(math.sin(2*math.pi*freq*(i/frate)))
  return numpy.array(samples)
  
def encode(floats):
  #floats = numpy.hanning(len(floats)) * floats
  shorts = [32767 * f for f in floats]
  return pack("%dh" % len(shorts), *shorts)

def sendChar(c, carrier, stream):
  # Whether each audio signal is high, in pairs
  carrier_bits = [0] * 2
  if carrier:
    carrier_bits[0] = 1
  else:
    carrier_bits[1] = 1
  
  signal_bits = [] # lsb first, in pairs
  print "'%s' (%d)" % (c, ord(c))
  c = ord(c)
  for i in xrange(7):
    if (c >> i) % 2:
      signal_bits.extend([1, 0])
    else:
      signal_bits.extend([0, 1])

  print carrier_bits, signal_bits

  waveforms = []
  if carrier_bits[0]: waveforms.append(sinewave(F_BASE - 2*F_SPACE, CHUNK))
  if carrier_bits[1]: waveforms.append(sinewave(F_BASE - F_SPACE, CHUNK))
  f = F_BASE
  for bit in signal_bits:
    if bit:
      waveforms.append(sinewave(f, CHUNK))
    f += F_SPACE

  #print "summing", len(waveforms), "waveforms with", len(waveforms[0]), "samples"
  agg = sum(waveforms)
  #print "summed:", agg
  agg = agg / max(agg)
  #print "normalized:", agg
  #print 
  #print "**begin transmit"
  stream.write(encode(agg))
  #print "end transmit"
  

def main():
  stream = openStream()

  carrier = False
  while True:
    c = getch()
    if (not c) or ord(c) == 27:
      break
    sendChar(c, carrier, stream)
    carrier = not carrier
    
  sendChar('\0', carrier, stream)

def getch():
  fd = sys.stdin.fileno()
  old_settings = None
  try:
    old_settings = termios.tcgetattr(fd)
    tty.setraw(sys.stdin.fileno())
    ch = sys.stdin.read(1)
  except:
    ch = sys.stdin.read(1)
  finally:
    if old_settings is not None:
      termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
  return ch


main()

