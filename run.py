#!/usr/bin/env python

import binascii
import math
import numpy as np
import random
import sys
import time
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
  parser.add_option('-e', '--redundancy', default=5.0, type='float',
      help='Encoder redundancy ratio (encoder-dependent)')
  parser.add_option('-n', '--numchans', default=16, type='int',
      help='Number of frequency channels')
  parser.add_option('-t', '--test', action='store_true',
      help='Run in test mode with known data')
  parser.add_option('-v', '--verbose', action='store_true')

  return parser.parse_args()

# Bytes per packet, until a header advertises a length
PACKET_DATA_BYTES = 10
USE_SYNC = True


def main():
  (options, args) = parseArgs()

  # Fixme: calculate width from the assigner, bitsPerChip
  #assigner = PairwiseAssigner(options.numchans)
  assigner = CombinadicAssigner(options.numchans)
  encoder = MajorityEncoder(options.redundancy, assigner.bitsPerChip())
  packeter = Packeter(encoder, assigner)

  chipDuration = 1.0 / options.rate
  chipSamples = int(SAMPLE_RATE * chipDuration)
  bitsPerChip = packeter.bitsPerChip()
  bitsPerSecond = bitsPerChip * options.rate
  chipsPerByte = packeter.symbolsForBytes(100)/100.0
  byteRate = options.rate / chipsPerByte

  subcarrierSpacing = int(SAMPLE_RATE / chipSamples)
  assert subcarrierSpacing == int(1.0 / chipDuration)
  channelGap = subcarrierSpacing * options.spacing

  baseBucket = options.base * chipSamples / SAMPLE_RATE
  syncer = SyncUtil(baseBucket, options.spacing, options.numchans / 2)

  if options.base % subcarrierSpacing != 0:
    print "Base", options.base, "Hz is not a subcarrier multiple, rounding down"
    options.base -= (options.base % subcarrierSpacing)

  print chipSamples / 2, "FFT buckets, subcarrier spacing", subcarrierSpacing, "Hz,", \
      "channel spacing", options.spacing
  print "Base frequency", options.base, "Hz,", options.numchans, "channels, total bandwidth", \
    options.numchans * channelGap, "Hz"
  print "Chip rate", options.rate, "Hz,", "duration", int(chipDuration * 1000), "ms,", \
      chipSamples, "samples/chip"
  print options.redundancy, "encoder redundancy,", bitsPerChip, "bits/chip,", bitsPerSecond, \
      "bits/sec"

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

    # combine a decent chunk or we get a buffer underrun.
    def createSyncSignal(chipsPerPulse):
      p = pattern

      def createPulse(pattern):
        waveforms = []
        f = base
        for bit in pattern:
          if bit:
            waveforms.append(sinewave(f, chipsPerPulse*chipSamples))
          f += channelGap
        return list(combine(waveforms))

      return createPulse(pattern) + createPulse(opposite)

    chipsPerLongPulse = 3
    metaSignalBucket = 2
    syncLong = createSyncSignal(chipsPerLongPulse) * 8
    syncReady = createSyncSignal(1) * chipsPerLongPulse * metaSignalBucket

    # silence is not actually necessary, it's just to prevent
    # buffer underruns with the audio output since we're not
    # sending any data here
    # FIXME this shouldn't be necessary
    silenc = list(silence(chipSamples * 3))

    sender.sendWaveForm(np.array(syncLong + syncReady + silenc))

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
        if biterrs:
          print "Data corrupt! %d of %d bits wrong (%.2f)" % (biterrs, nbits, 1.0*biterrs/nbits)
        else:
          print PACKET_DATA_BYTES, "bytes ok"
      else:
        sys.stdout.write(s)
        sys.stdout.flush()


  def testSync(receiver):
    syncer.align(receiver, chipSamples)

    # Alex: now aligned. Start reading data chips at the chip rate.
    print "ALIGNED"
    sys.exit()


  def sendPacket(data, sender):
    assert len(data) == PACKET_DATA_BYTES, "data length must match packet length"

    sendPreamble(sender)

    chips = packeter.encodePacket(data)
    #print "chips:", chips
    print "data: '%s'" % binascii.hexlify(data)
    for chip in chips:
      if options.verbose: print "chip: %s" % (chip)
      waveform = buildWaveform(chip, options.base, channelGap, chipSamples)
      if chip is chips[0]:
        fadein(waveform, int(chipSamples / 20))
      if chip is chips[-1]:
        fadeout(waveform, int(chipSamples / 20))
      sender.sendWaveForm(waveform)

  # Compute a signal with energy at each frequency corresponding to a
  # non-zero number in chip
  def buildWaveform(chip, base, spacing, samples):
    freqs = [ base + spacing * i for i in xrange(len(chip)) if chip[i] > 0 ]
    return sinewaves(freqs, samples)

  # Receives a packet, being a marker followed
  # by data. This is a placeholder for real marker detection to come.
  def receivePacket(receiver):
    receivePreamble(receiver)

    packetChips = []
    numPacketSymbols = packeter.symbolsForBytes(PACKET_DATA_BYTES)
    #print "expecting", numPacketSymbols, "chips for", PACKET_DATA_BYTES, "bytes"
    for symbolIndex in xrange(numPacketSymbols):
      waveform = receiver.receiveBlock(chipSamples)
      spectrum = fouriate(waveform)

      chip = []
      for f in xrange(options.base, options.base + (channelGap*options.numchans), channelGap):
        chip.append(spectrum.power(f))

      #print "*** chip", chip
      packetChips.append(chip)
      
    data = packeter.decodePacket(packetChips)

    # Finished receiving packet
    if options.verbose: 
      print "\n== packet done, bit error rate %.3f ==" % (packeter.lastErrorRate())

    return data

  # Sends a packet preamble, signals to allow the receiver to align
  def sendPreamble(sender):
    if USE_SYNC:
      sendSync(sender)
    else:
      # First send marker, which is just one symbol length of the old carrier in False polarity
      headerBase = options.base - 2 * channelGap # carrier goes below base
      bitstring = [1, 0]
      headerSamples = int(chipSamples * options.redundancy)
      header = buildWaveform(bitstring, headerBase, channelGap, headerSamples)
      fadein(header, chipSamples / 20)
      fadeout(header, chipSamples / 20)
      sender.sendWaveForm(header)


  # Receives a packet preamble, leaving the audio stream aligned to receive
  # the first data chip.
  def receivePreamble(receiver):
    if USE_SYNC:
      syncer.align(receiver, chipSamples)
    else:
      # Minimum SNR to detect a marker signal (dB)
      minPreambleSnr = 6.0

      # Each channel uses 2 subcarriers
      chandiff = 2 * channelGap

      markerChipsRequired = int(options.redundancy * 0.8)
      markerChipsReceived = 0

      while markerChipsReceived < markerChipsRequired:
        waveform = receiver.receiveBlock(chipSamples)
        spectrum = fouriate(waveform)

        heartA = spectrum.power(options.base - chandiff)
        heartB = spectrum.power(options.base - chandiff + channelGap)
        snr = abs(dbPower(heartA, heartB))

        if snr > minPreambleSnr:
          if heartA > heartB:
            markerChipsReceived += 1
            if options.verbose: print "+",
          else:
            markerChipsReceived = 0
            if options.verbose: print "-",
        else:
          pass

    # Preamble finished!
    if options.verbose: print



  if options.send:
    doSend()
  elif options.listen:
    doListen()


def genTestData(nbytes):
  random.seed(2)
  return ''.join( (chr(random.randint(0, 255)) for i in xrange(nbytes)) )


if __name__ == "__main__":
  main()
