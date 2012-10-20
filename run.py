#!/usr/bin/env python

import sys
import math
import numpy as np
import random
from optparse import OptionParser

from util import *
from audio import *
from packet import *
from sync import SyncUtil

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
  parser.add_option('-n', '--numchans', default=16, type='int',
      help='Number of frequency channels')
  parser.add_option('-t', '--test', action='store_true',
      help='Run in test mode with known data')
  parser.add_option('-v', '--verbose', action='store_true')

  return parser.parse_args()

# Symbols per packet, until a header advertises a length
PACKET_DATA_BYTES = 10


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

  packeter = Packeter(IdentityEncoder(), PairwiseAssigner(options.numchans))

  def doSend():
    sender = PyAudioSender()
    carrier = False
    eof = False
    while not eof:
      # Uncomment to test sending sync signal
      #sendSync(sender)

      if options.test:
        chars = genTestData(PACKET_DATA_BYTES)
      else:
        # Terminal input will be line-buffered, but read PACKET bytes at a time.
        chars = sys.stdin.read(PACKET_DATA_BYTES)
        if not chars:
          eof = True

      if chars:
        sendPacket(chars, sender)

  def sendSync(sender):
    base = options.base

    assert options.numchans % 2 == 0
    pattern = [1,0,1,0] * (options.numchans / 8)
    opposite = [1-b for b in pattern]
    p = pattern

    data = []
    # combine a decent chunk or we get a buffer underrun.
    for i in xrange(20):
      p = (p == opposite) and pattern or opposite
      waveforms = []
      f = base
      for bit in p:
        if bit:
          waveforms.append(sinewave(f, 2*chipSamples))
        f += channelGap

      data += list(combine(waveforms))
    while True:

      sender.sendWaveForm(np.array(data))

  def doListen():
    if options.stdin:
      receiver = StreamReceiver(sys.stdin)
    else:
      receiver = PyAudioReceiver()
    while True:
      #testSync(receiver)
      s = receivePacket(receiver)
      if options.test:
        expected = genTestData(PACKET_DATA_BYTES)
        nbits = PACKET_DATA_BYTES * 8
        biterrs = sum(map(lambda t: countbits(ord(t[0]) ^ ord(t[1])), zip(s, expected)))
        print "Bit errors %d/%d (%.3f)" % (biterrs, nbits, 1.0*biterrs/nbits)
      else:
        sys.stdout.write(s)
        sys.stdout.flush()



  def testSync(receiver):
    baseBucket = options.base * chipSamples / SAMPLE_RATE
    sync = SyncUtil(baseBucket, options.spacing, options.numchans / 2)
    sync.align(receiver, chipSamples)


  def sendPacket(data, sender):
    assert len(data) == PACKET_DATA_BYTES, "data length must match packet length"

    # First send marker, which is just one symbol length of the old carrier in False polarity
    # TODO: replace this with sync signal
    headerBase = options.base - 2 * channelGap # carrier goes below base
    bitstring = [1, 0]
    sender.sendWaveForm(buildWaveform(bitstring, headerBase, channelGap))

    chips = packeter.encodePacket(data)
    #print "chips:", chips
    print "data: '%s'" % data
    for chip in chips:
      print "chip: %s" % (chip)
      sender.sendWaveForm(buildWaveform(chip, options.base, channelGap))

  # Compute a signal with energy at each frequency corresponding to a
  # non-zero number in chip
  def buildWaveform(chip, baseFrequency, frequencySpacing):
    # TODO: consider using inverse FFT to build this
    waveforms = []
    f = baseFrequency
    for channel in chip:
      if channel > 0:
        waveforms.append(sinewave(f, symbolSamples))
      f += frequencySpacing
    return combine(waveforms)


  # Receives a packet, being a marker followed
  # by data. This is a placeholder for real marker detection to come.
  def receivePacket(receiver):
    # Minimum SNR to detect a marker signal (dB)
    minMarkerSnr = 6.0

    # Each channel uses 2 subcarriers
    chandiff = 2 * channelGap

    markerChipsRequired = int(options.symbolchips * 0.8)
    markerChipsReceived = 0

    # First wait for a marker
    while markerChipsReceived < markerChipsRequired:
      waveform = receiver.receiveBlock(chipSamples)
      spectrum = fouriate(waveform)

      heartA = spectrum.power(options.base - chandiff)
      heartB = spectrum.power(options.base - chandiff + channelGap)
      # Note: the SNR is a bit BS because we don't actually know if there's a signal
      markerSnr = abs(dbPower(heartA, heartB))

      if markerSnr > minMarkerSnr:
        if heartA > heartB:
          markerChipsReceived += 1
          if options.verbose: print "+",
        else:
          markerChipsReceived = 0
          if options.verbose: print "-",
      else:
        pass

    # Marker finished!
    if options.verbose: print
    numCarriers = options.numchans
    packetChips = []
    packetSnrs = []
    # FIXME calculate this from knowledge of underlying encoder/assigner
    packetSymbols = PACKET_DATA_BYTES
    for symbolIndex in xrange(packetSymbols):
      # Array of # carriers x # chips of channel values
      subcarrierSeqs = []
      # SNRs for chips in this symbol
      chipSnrs = []
      for i in xrange(numCarriers):
        subcarrierSeqs.append([])
      for i in xrange(options.symbolchips):
        waveform = receiver.receiveBlock(chipSamples)
        spectrum = fouriate(waveform)

        # Sum of signal and noise power in this chip
        signal = 0
        noise = 0
        # look at pairs of subcarriers to compare them and record bit likelihoods
        # as -1 or 1
        for carrierIdx in xrange(0, numCarriers, 2):
          ia = spectrum.power(options.base + carrierIdx * channelGap)
          ib = spectrum.power(options.base + (carrierIdx + 1) * channelGap)
          if ia > ib:
            subcarrierSeqs[carrierIdx].append(1.0)
            subcarrierSeqs[carrierIdx + 1].append(-1.0)
            signal += ia
            noise += ib
          else:
            subcarrierSeqs[carrierIdx].append(-1.0)
            subcarrierSeqs[carrierIdx + 1].append(1.0)
            signal += ib
            noise += ia
        chipSnrs.append(dbPower(signal, noise))

      # Compute a superchip as the mean bit likelihoods of the chips
      superChip = map(np.mean, subcarrierSeqs)
      #print "chip", superChip
      snr = np.average(chipSnrs)
      packetSnrs.append(snr)
      #print chipSnrs
      #print "SNR avg: %.1f, worst: %.1f" % (snr, min(chipSnrs))
      packetChips.append(superChip)
      
    # Finished receiving packet
    if options.verbose: 
      print "\n== packet done, SNR: %.1f, worst: %.1f==" % (np.average(packetSnrs), min(packetSnrs))

    return packeter.decodePacket(packetChips)


  if options.send:
    doSend()
  elif options.listen:
    doListen()


def genTestData(nbytes):
  random.seed(1)
  return ''.join( (chr(random.randint(0, 255)) for i in xrange(nbytes)) )


if __name__ == "__main__":
  main()
