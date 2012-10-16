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
  parser.add_option('-v', '--verbose', action='store_true')

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
    eof = False
    while not eof:
      # Terminal input will be line-buffered, but read 1 byte at a time.
      chars = sys.stdin.read(1)
      for c in chars:
        sendChar(c, carrier, sender)
        carrier = not carrier
      if not chars:
        eof = True


  def doListen():
    if options.stdin:
      receiver = StreamReceiver(sys.stdin)
    else:
      receiver = PyAudioReceiver()
    while True:
      # Use one of these at a time
      listen(receiver)
      #receivePacket(receiver)


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
  # Minimum SNR to detect a heartbeat signal (dB)
  minHeartbeatSnr = 4.0

  class State:
    pass

  x = State()
  x.count = 0
  x.bitsignals = [[0] * (int(winFactor * options.symbolchips))] * options.numchans
  x.heartSignals = [0] * options.symbolchips
  x.lastHeart = 0
  # Moving average signal and noise
  x.noise = MovingAverage(options.symbolchips * 2)
  x.signal = MovingAverage(options.symbolchips * 2)

  def listen(receiver):
    x.count += 1
    waveform = receiver.receiveBlock(chipSamples)
    spectrum = fouriate(waveform)

    # Each channel uses 2 subcarriers
    chandiff = 2 * channelGap
    for i in range(0, options.numchans):
      ia = spectrum.power(options.base + i * chandiff)
      ib = spectrum.power(options.base + i * chandiff + channelGap)
      x.bitsignals[i] = x.bitsignals[i][1:] + [ia > ib and 1 or 0]

    heartA = spectrum.power(options.base - chandiff)
    heartB = spectrum.power(options.base - chandiff + channelGap)
    # Note: the SNR is a bit BS because we don't actually know if there's a signal
    heartbeatSnr = abs(dbPower(heartA, heartB))
    x.noise.sample(min(heartA, heartB))
    x.signal.sample(max(heartA, heartB))

    if heartbeatSnr > minHeartbeatSnr:
      if (heartA > heartB):
        x.heartSignals = x.heartSignals[1:] + [1]
        if options.verbose: print '_',
      else:
        x.heartSignals = x.heartSignals[1:] + [-1]
        if options.verbose: print 'X',
    else:
      x.heartSignals = x.heartSignals[1:] + [0]

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
      #if options.verbose: print '================'
      for s in x.bitsignals:
        if np.mean(s) >= 0.5:
          #if options.verbose: print 1,
          v += pos
        else:
          #if options.verbose: print 0,
          pass
        pos *= 2
      if options.verbose: print

      if v != 0: 
        if v == 13: print
        if options.verbose: print '=================>', chr(v), '<================='
        else: sys.stdout.write(chr(v))
    if options.verbose and not (x.count % (options.symbolchips * 3)):
      print "SNR: %.1fdB" % dbPower(x.signal.avg(), x.noise.avg())


    sys.stdout.flush()

  # WIP alternatve to listen() that receives a packet, being a marker followed
  # by data. This is a placeholder for real marker detection to come.
  def receivePacket(receiver):
    # Each channel uses 2 subcarriers
    chandiff = 2 * channelGap

    markerChipsRequired = int(options.symbolchips * 0.8)
    markerChipsReceived = 0
    markerPolarity = False

    # First wait for a marker
    while markerChipsReceived < markerChipsRequired:
      waveform = receiver.receiveBlock(chipSamples)
      spectrum = fouriate(waveform)

      heartA = spectrum.power(options.base - chandiff)
      heartB = spectrum.power(options.base - chandiff + channelGap)
      # Note: the SNR is a bit BS because we don't actually know if there's a signal
      heartbeatSnr = abs(dbPower(heartA, heartB))

      if heartbeatSnr > minHeartbeatSnr:
        polarity = heartA > heartB
        if polarity == markerPolarity:
          markerChipsReceived += 1
          if options.verbose: print "+",
        else:
          markerChipsReceived = 0
          markerPolarity = polarity
          if options.verbose: print "-",
      else:
        pass

    # Marker finished!
    if options.verbose: print
    bitsignals = [[0] * (options.symbolchips)] * options.numchans
    for si in xrange(20):
      for i in xrange(options.symbolchips):
        waveform = receiver.receiveBlock(chipSamples)
        spectrum = fouriate(waveform)

        for ci in range(0, options.numchans):
          ia = spectrum.power(options.base + ci * chandiff)
          ib = spectrum.power(options.base + ci * chandiff + channelGap)
          bitsignals[ci] = bitsignals[ci][1:] + [ia > ib and 1 or 0]

      # All chips for a symbol received, output best-effort data
      received = 0
      bit = 1
      for s in bitsignals:
        if np.mean(s) >= 0.5:
          #if options.verbose: print 1,
          received += bit
        else:
          #if options.verbose: print 0,
          pass
        bit <<= 1
      sys.stdout.write(chr(received))

    # Finished receiving packet
    if options.verbose: print "\n== packet done =="


  if options.send:
    doSend()
  elif options.listen:
    doListen()





if __name__ == "__main__":
  main()
