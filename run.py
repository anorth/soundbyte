#!/usr/bin/env python

import binascii
import logging
import math
import numpy as np
import random
import sys
import time
import threading
from optparse import OptionParser

from util import *
from audio import *
from packet import *
from mixer import *
from sync import genSync, SyncUtil

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
  parser.add_option('-R', '--syncrate', default=200, type='int',
      help='Sync chip rate (hz)')
  parser.add_option('-e', '--redundancy', default=5.0, type='float',
      help='Encoder redundancy ratio (encoder-dependent)')
  parser.add_option('-n', '--numchans', default=16, type='int',
      help='Number of frequency channels')
  parser.add_option('-N', '--numsyncchans', default=8, type='int',
      help='Number of sync frequency channels')
  parser.add_option('-t', '--test', action='store_true',
      help='Run in test mode with known data')
  parser.add_option('--selftest', action='store_true',
      help='Run in self-test mode with both sender and receiver')
  parser.add_option('-f', '--file', type='str',
      help='Noise WAV file path')
  parser.add_option('--signal', type='int', default="-6",
      help='Mixer signal strength, dB below unity')
  parser.add_option('--noise', type='int', default="-6",
      help='Mixer noise strength, dB below unity')
  parser.add_option('-v', '--verbose', action='store_true')

  (opts, args) = parser.parse_args()
  if opts.send and opts.listen:
    assert False, "Use selftest to send and listen"
  if opts.selftest:
    opts.test = True
    opts.send = True
    opts.listen = True
  return (opts, args)

# Bytes per packet, until a header advertises a length
PACKET_DATA_BYTES = 20


def main():
  (options, args) = parseArgs()

  lvl = options.verbose and logging.DEBUG or logging.INFO
  logging.basicConfig(stream = sys.stderr, level = lvl, format = "%(message)s")

  assigner = CombinadicAssigner(options.numchans)
  encoder = RepeatingEncoder(options.redundancy, assigner.bitsPerChip())
  packeter = Packeter(encoder, assigner)

  chipDuration = 1.0 / options.rate
  chipSamples = int(SAMPLE_RATE / options.rate)
  bitsPerChip = packeter.bitsPerChip()
  bitsPerSecond = bitsPerChip * options.rate
  chipsPerByte = packeter.symbolsForBytes(100)/100.0
  byteRate = options.rate / chipsPerByte

  subcarrierSpacing = int(SAMPLE_RATE / chipSamples)
  assert subcarrierSpacing == int(1.0 / chipDuration)
  channelGap = subcarrierSpacing * options.spacing

  baseBucket = options.base * chipSamples / SAMPLE_RATE

  syncRatio = (options.syncrate / options.rate)
  syncChipSamples = int(SAMPLE_RATE / options.syncrate)
  syncChannelGap = channelGap * syncRatio
  numSyncChans = options.numsyncchans
  syncBaseBucket = options.base * syncChipSamples / SAMPLE_RATE
  assert numSyncChans >= 4 # want at least 4
  syncer = SyncUtil(syncBaseBucket, options.spacing, numSyncChans, verbose = options.verbose)

  logging.info("num sync chans: %d" % numSyncChans)

  if options.base % subcarrierSpacing != 0:
    logging.info("Base %d Hz is not a subcarrier multiple, rounding down" % options.base)
    options.base -= (options.base % subcarrierSpacing)

  logging.info("%d FFT buckets, subcarrier spacing %d Hz, channel spacing %d" %
      (chipSamples / 2, subcarrierSpacing, options.spacing))
  logging.info("Base frequency %d Hz, top frequency %d, %d channels, total bandwidth %d Hz" % 
      (options.base, options.base + options.numchans * channelGap, options.numchans, 
        options.numchans * channelGap))
  logging.info("Chip rate %d Hz, duration %d ms, %d samples/chip" % 
      (options.rate, int(chipDuration * 1000), chipSamples))
  logging.info("%d encoder redundancy, %.1f raw bits/chip, %.1f corrected bits/chip, %d bits/sec" % \
      (options.redundancy, assigner.bitsPerChip(), bitsPerChip, bitsPerSecond))

  class SelfTest:
    infile = None
    outfile = None

  class Control:
    running = True
    packetsSent = 0
    packetsReceived = 0
    packetsCorrupt = 0
    bitErrors = 0

  def doSend():
    f = None
    if options.selftest:
      sender = StreamSender(SelfTest.outfile)
      f = SelfTest.outfile
    elif options.stdin:
      sender = StreamSender(sys.stdout)
      f = sys.stdout
    else:
      sender = PyAudioSender()
    eof = False
    try:
      while Control.running and not eof:
        if options.test:
          chars = genTestData(PACKET_DATA_BYTES)
        else:
          # Terminal input will be line-buffered, but read PACKET bytes at a time.
          chars = sys.stdin.read(PACKET_DATA_BYTES)
          if not chars:
            eof = True

        if chars:
          sendPacket(chars, sender)
          # Send a 1-chip silence buffer
          sender.sendBlock(silence(chipSamples))
          Control.packetsSent += 1
    except Exception, e:
      logging.info("%s" % e)
    if f:
      f.close()

  def doListen():
    if options.selftest:
        receiver = StreamReceiver(SelfTest.infile)
    elif options.stdin:
      receiver = StreamReceiver(sys.stdin)
    else:
      receiver = PyAudioReceiver()
    if options.file:
      receiver = NoiseMixingReceiver(receiver, options.file, options.signal, options.noise)
    try:
      totalBitErrs = 0
      while True:
        s = receivePacket(receiver)
        if options.test:
          expected = genTestData(PACKET_DATA_BYTES)
          nbits = PACKET_DATA_BYTES * 8
          biterrs = sum(map(lambda t: countbits(ord(t[0]) ^ ord(t[1])), zip(s, expected)))
          if biterrs:
            Control.bitErrors += biterrs
            Control.packetsCorrupt += 1
            logging.info("-> Data corrupt! %d of %d bits wrong (%.2f)" % (biterrs, nbits, 1.0*biterrs/nbits))
          else:
            logging.debug("-> %d bytes ok" % len(s))
          Control.packetsReceived += 1
        else:
          sys.stdout.write(s)
          sys.stdout.flush()
    except BaseException, e:
      logging.info("%s" % e)

  def sendPacket(data, sender):
    assert len(data) == PACKET_DATA_BYTES, "data length must match packet length"

    chips = packeter.encodePacket(data)

    logging.debug("data: '%s'" % binascii.hexlify(data))
    preamble = genPreamble()
    final = list(preamble)
    for chip in chips:
      logging.debug("chip: %s" % (chip))
      waveform = buildWaveform(chip, options.base, channelGap, chipSamples)
      if chip is chips[0]:
        fadein(waveform, int(chipSamples / 20))
      if chip is chips[-1]:
        fadeout(waveform, int(chipSamples / 20))
#      if len(waveforms) == 0:
#        waveform = list(genPreamble()) + list(waveform)
      final.extend(waveform)

    sender.sendBlock(final)
    #time.sleep(2)
    #print '==== BEGIN ===='
    #for waveform in waveforms:
    #  print len(waveform),
    #  #print 'HELLO'
    #  sender.sendBlock(waveform)
    #  print 'x',
    #print '\n==== END ===='


  # Receives a packet, being a marker followed
  # by data. This is a placeholder for real marker detection to come.
  def receivePacket(receiver):
    receivePreamble(receiver)

    packetChips = []
    numPacketSymbols = packeter.symbolsForBytes(PACKET_DATA_BYTES)
    #print "expecting", numPacketSymbols, "chips for", PACKET_DATA_BYTES, "bytes"
    for symbolIndex in xrange(numPacketSymbols):
      waveform = receiver.receiveBlock(chipSamples)
      spectrum = fouriate(window(waveform))

      chip = []
      for f in xrange(options.base, options.base + (channelGap*options.numchans), channelGap):
        chip.append(spectrum.power(f))

      packetChips.append(chip)
      
    data = packeter.decodePacket(packetChips)

    # Finished receiving packet
    logging.info("Packet received, SNR %.2f dB, detected raw bit error rate %.3f" % 
        (assigner.lastSignalRatio(), packeter.lastErrorRate()))

    return data

  # Sends a packet preamble, signals to allow the receiver to align
  def genPreamble():
    return genSync(options.base, numSyncChans, syncChannelGap, syncChipSamples)


  # Receives a packet preamble, leaving the audio stream aligned to receive
  # the first data chip.
  def receivePreamble(receiver):
    syncer.align(receiver, syncChipSamples)

  if options.selftest:
    (ii, oo) = socket.socketpair()
    SelfTest.infile = ii.makefile('rb')
    SelfTest.outfile = oo.makefile('wb')

  if options.send:
    t = threading.Thread(target = doSend, name = "sendThread")
    t.daemon = True
    t.start()
  if options.listen:
    t = threading.Thread(target = doListen, name = "listenThread")
    t.daemon = True
    t.start()

  # Wait for KeyboardInterrupt. Note that thread.join() is uninterruptable.
  try:
    while True: time.sleep(1000)
  except KeyboardInterrupt:
    pass
  except BaseException, e:
    logging.info("%s" % type(e))

  Control.running = False
  time.sleep(1)
  if options.send:
    logging.info("=> %d packets sent" % Control.packetsSent)
  if options.listen:
    logging.info("=> %d packets received" % Control.packetsReceived)
    logging.info("=> %d uncorrected bit errors" % Control.bitErrors)
    logging.info("=> %d packets corrupt" % Control.packetsCorrupt)
  if options.selftest:
    logging.info("=> %d packets dropped" % (Control.packetsSent - Control.packetsReceived))


def genTestData(nbytes):
  random.seed(2)
  return ''.join( (chr(random.randint(0, 255)) for i in xrange(nbytes)) )


if __name__ == "__main__":
  main()
