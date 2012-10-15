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
  parser.add_option('-p', '--spacing', default=2, type='int',
      help='Channel spacing (# subcarriers)')
  parser.add_option('-r', '--rate', default=100, type='int',
      help='Chip rate (hz)')
  parser.add_option('-c', '--symbolchips', default=10, type='int',
      help='Chips per symbol')
  parser.add_option('-n', '--numchans', default=7, type='int',
      help='Number of frequency channels')

  return parser.parse_args()


def main():
  (options, args) = parseArgs()

  chipDuration = 1.0 / options.rate
  chipSamples = int(SAMPLE_RATE * chipDuration)

  symbolRate = options.rate / options.symbolchips
  symbolDuration = chipDuration * options.symbolchips
  symbolSamples = chipSamples * options.symbolchips

  subcarrierSpacing = int(SAMPLE_RATE / chipSamples)
  assert subcarrierSpacing == int(1.0 / chipDuration)
  channelGap = subcarrierSpacing * options.spacing

  if options.base % subcarrierSpacing != 0:
    print "Base", options.base, "Hz is not a subcarrier multiple, rounding down"
    options.base -= (options.base % subcarrierSpacing)

  print "Chip rate", options.rate, "Hz,", "duration", int(chipDuration * 1000), "ms,", \
      chipSamples, "samples/chip"
  print "Symbol rate", symbolRate, "Hz,", "duration", int(symbolDuration * 1000), "ms,", \
      options.symbolchips, "chips/symbol"
  print chipSamples / 2, "FFT buckets, subcarrier spacing", subcarrierSpacing, "Hz,", \
      "channel spacing", options.spacing
  print "Base frequency", options.base, "Hz,", options.numchans, "channels, total bandwidth", \
    options.numchans * channelGap, "Hz"

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
    if carrier_bits[0]: waveforms.append(sinewave(options.base - 2*channelGap, symbolSamples))
    if carrier_bits[1]: waveforms.append(sinewave(options.base - channelGap, symbolSamples))
    f = options.base
    for bit in signal_bits:
      if bit:
        waveforms.append(sinewave(f, symbolSamples))
      f += channelGap

    sender.sendWaveForm(combine(waveforms))
  

  winFactor = 0.6

  class Crap:
    pass

  x = Crap()

  x.bitsignals = [[0] * (int(winFactor * options.symbolchips))] * options.numchans
  x.heartSignals = [0] * options.symbolchips

  x.lastHeart = 0

  def listen(receiver):
    shorts = receiver.receiveBlock(chipSamples)
    spectrum = fouriate(shorts)

    # Each channel uses 2 subcarriers
    chandiff = 2 * channelGap
    for i in range(0, options.numchans):
      ia = spectrum.amplitude(options.base + i * chandiff)
      ib = spectrum.amplitude(options.base + i * chandiff + channelGap)
      x.bitsignals[i] = x.bitsignals[i][1:] + [ia > ib and 1 or 0]

    heartA = spectrum.amplitude(options.base - chandiff)
    heartB = spectrum.amplitude(options.base - chandiff + channelGap)

    factor = 2.0

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
    if abs(heartSum) >= winFactor * options.symbolchips:
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
