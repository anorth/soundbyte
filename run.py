#!/usr/bin/env python

import sys
import math
import numpy as np
from optparse import OptionParser

from util import *
from audio import *

def parseArgs():
  parser = OptionParser()
  parser.add_option('-l', '--listen', action='store_true')
  parser.add_option('-s', '--send', action='store_true')
  parser.add_option('-i', '--stdin', action='store_true')
  parser.add_option('-b', '--base', default=15000, type='int',
      help='Base frequency (hz)')
  parser.add_option('-g', '--gap', default=200, type='int',
      help='Channel gap (hz)')
  parser.add_option('-r', '--rate', default=10, type='float',
      help='Chunks per second')
  parser.add_option('-n', '--numchans', default=7, type='int',
      help='Number of frequency channels')

  return parser.parse_args()


def main():
  (options, args) = parseArgs()

  chunkSize = SAMPLE_RATE / options.rate
  chunkTime = 1.0 / options.rate

  def doSend():
    sender = PyAudioSender()
    carrier = False
    while True:
      c = getch()
      if (not c) or ord(c) == 27:
        break
      sendChar(c, carrier, sender)
      carrier = not carrier

  def doListen():
    if options.stdin:
      receiver = StreamReceiver(sys.stdin)
    else:
      receiver = PyAudioReceiver()
    while True:
      listen(receiver)


  def sendChar(c, carrier, sender):
    # Whether each audio signal is high, in pairs
    carrier_bits = [0] * 2
    if carrier:
      carrier_bits[0] = 1
    else:
      carrier_bits[1] = 1
    
    signal_bits = [] # lsb first, in pairs
    print "'%s' (%d)" % (c, ord(c))
    c = ord(c)
    for i in xrange(options.numchans):
      if (c >> i) % 2:
        signal_bits.extend([1, 0])
      else:
        signal_bits.extend([0, 1])

    print carrier_bits, signal_bits

    waveforms = []
    if carrier_bits[0]: waveforms.append(sinewave(options.base - 2*options.gap, chunkSize))
    if carrier_bits[1]: waveforms.append(sinewave(options.base - options.gap, chunkSize))
    f = options.base
    for bit in signal_bits:
      if bit:
        waveforms.append(sinewave(f, chunkSize))
      f += options.gap

    sender.sendWaveForm(combine(waveforms))
  

  CHIPS_PER_CHUNK = 10
  winFactor = 0.6
  thresh = 20
  heartWin = int(CHIPS_PER_CHUNK)
  chipSize = chunkSize / CHIPS_PER_CHUNK
  chipTime = chunkTime / CHIPS_PER_CHUNK

  class Crap:
    pass

  x = Crap()

  x.bitsignals = [[0] * (int(winFactor * CHIPS_PER_CHUNK))] * options.numchans
  x.heartSignals = [0] * heartWin

  x.heartVal = 0
  x.lastHeart = 0

  def listen(receiver):
    shorts = receiver.receiveBlock(chipSize)
    spectrum = fouriate(shorts, chipTime)

    chandiff = 2 * options.gap
    assert options.gap * chipTime >= 2
    for i in range(0, options.numchans):
      ia = spectrum.intensity(options.base + i * chandiff)
      ib = spectrum.intensity(options.base + i * chandiff + options.gap)
      x.bitsignals[i] = x.bitsignals[i][1:] + [ia > ib and 1 or 0]

    heartA = spectrum.intensity(options.base - chandiff)
    heartB = spectrum.intensity(options.base - chandiff + options.gap)

    factor = 2.0

    inc = 3
    mx = thresh * inc
    dt = mx

    x.heartVal = min(mx, max(-mx, x.heartVal))

    if (heartA > heartB * factor):
      x.heartSignals = x.heartSignals[1:] + [1]
      #print '_',
    elif (heartB > heartA * factor):
      x.heartSignals = x.heartSignals[1:] + [-1]
      #print 'X',
    else:
      x.heartSignals = x.heartSignals[1:] + [0]
      #print ' ',
      pass

    gotBeat = False
    heartSum = sum(x.heartSignals)
    if abs(heartSum) >= winFactor * heartWin:
      h = np.sign(heartSum)
      if h != x.lastHeart:
        gotBeat = True
        x.lastHeart = np.sign(heartSum)


    if gotBeat:
      v = 0
      pos = 1
      #print '=============================='
      for s in x.bitsignals:
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

  if options.send:
    doSend()
  elif options.listen:
    doListen()





if __name__ == "__main__":
  main()
