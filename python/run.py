#!/usr/bin/env python

import binascii
import logging
import math
import md5
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
  parser.add_option('-r', '--rate', default=50, type='int',
      help='Chip rate (hz)')
  parser.add_option('-R', '--syncrate', default=200, type='int',
      help='Sync chip rate (hz)')
  parser.add_option('-e', '--redundancy', default=2.0, type='float',
      help='Encoder redundancy ratio (encoder-dependent)')
  parser.add_option('-n', '--numchans', default=8, type='int',
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
  parser.add_option('--encoder', type='string', default="rs",
      help='Encoder to use (lay, rs, repeat)')
  parser.add_option('--play', action='store_true',
      help='Play back received audio')
  parser.add_option('--repeat', type='int', default=1000000,
      help='Number times to send the data')
  parser.add_option('--csv', action='store_true',
      help='Generate CSV output')
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
CHUNK_DATA_BYTES = 20
CHUNK_DATA_BYTES = 10


def main():
  (options, args) = parseArgs()

  lvl = options.verbose and logging.DEBUG or logging.INFO
  logging.basicConfig(stream = sys.stderr, level = lvl, format = "%(message)s")

  assigner = PairwiseAssigner(options.numchans)
  assigner = CombinadicAssigner(options.numchans)
  if options.encoder == 'lay':
    encoder = LayeredEncoder(assigner.bitsPerChip(),
      CHUNK_DATA_BYTES,
      max(1.1, float(options.redundancy) / 3))
  elif options.encoder == 'rs':
    encoder = ReedSolomonEncoder(assigner.bitsPerChip(),
      int(options.redundancy * CHUNK_DATA_BYTES),
      CHUNK_DATA_BYTES)
  elif options.encoder == 'repeat':
    encoder = RepeatingEncoder(options.redundancy, assigner.bitsPerChip())
  else:
    assert False, 'invalid encoder %s' % options.encoder

  packeter = Packeter(encoder, assigner)

  chipDuration = 1.0 / options.rate
  chipSamples = int(SAMPLE_RATE / options.rate)
  bitsPerChip = packeter.bitsPerChip(CHUNK_DATA_BYTES)
  bitsPerSecond = bitsPerChip * options.rate
  #chipsPerByte = packeter.symbolsForBytes(100)/100.0
  #byteRate = options.rate / chipsPerByte

  subcarrierSpacing = int(SAMPLE_RATE / chipSamples)
  assert subcarrierSpacing == int(1.0 / chipDuration)
  channelGap = subcarrierSpacing * options.spacing

  baseBucket = options.base * chipSamples / SAMPLE_RATE

  syncRatio = float(options.syncrate) / options.rate
  syncChipSamples = int(SAMPLE_RATE / options.syncrate)
  syncChannelGap = channelGap * syncRatio
  assert syncChannelGap == math.floor(syncChannelGap)
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
    bitsCorrupt = 0

  def doSend():
    if options.test:
      data = genTestData(CHUNK_DATA_BYTES)
    else:
      data = sys.stdin.read()
    outstream = None
    iterations = options.repeat
    if options.selftest:
      sender = StreamSender(SelfTest.outfile)
      outstream = SelfTest.outfile
    elif options.stdin:
      sender = StreamSender(sys.stdout)
      outstream = sys.stdout
    else:
      sender = PyAudioSender()

    while Control.running and iterations > 0:
      iterations -= 1
      sendPacket(sender, data)
      Control.packetsSent += 1
    Control.running = False
    if outstream:
      outstream.close()

  def doListen():
    if options.selftest:
        receiver = StreamReceiver(SelfTest.infile)
    elif options.stdin:
      receiver = StreamReceiver(sys.stdin)
    else:
      receiver = PyAudioReceiver()
    if options.file:
      receiver = NoiseMixingReceiver(receiver, options.file, options.signal, options.noise)
    if options.play:
      receiver = PlaybackReceiver(receiver)
    #try:
    totalBitErrs = 0
    while True:
      (s, chips) = receivePacket(receiver)

      if options.test:
        break
        # XXX get this working again

        expected = genTestData(CHUNK_DATA_BYTES)
        expectedChips = packeter.encodePacket(expected)
        assert len(chips) == len(expectedChips)
        assert len(chips[0]) == len(expectedChips[0])

        byChannel = {}
        byChip = {}
          
        for chip in xrange(len(chips)):
          for chan in xrange(len(chips[0])):
            if chips[chip][chan] != expectedChips[chip][chan]:
              byChip[chip] = byChip.get(chip, []) + [chan]
              byChannel[chan] = byChannel.get(chan, 0) + 1

        if byChannel or byChip:
          logging.debug('Raw errors by Chip: ' + str(
            [(c, byChip.get(c)) for c in xrange(len(chips)) if byChip.get(c)]))
          logging.debug('Raw errors by Chan: ' + str(
            [byChannel.get(c, 0) for c in xrange(len(chips[0]))]))

      if s is None:
        continue

      if options.test:
        expected = genTestData(CHUNK_DATA_BYTES)
        nbits = CHUNK_DATA_BYTES * 8
        biterrs = sum(map(lambda t: countbits(ord(t[0]) ^ ord(t[1])), zip(s, expected)))
        byteErrors = []
        nbytes = CHUNK_DATA_BYTES

        # XXX HACK
        # use this instead! 
        # assert len(s) == len(expected), '%s %s' % (len(s), len(expected))
        if len(s) > len(expected):
          s = s[:len(expected)]
        # end XXX

        for c in xrange(len(expected)):
          if s[c] != expected[c]:
            byteErrors.append(c)
        nByteErrors = len(byteErrors)

        if biterrs:
          Control.bitsCorrupt += biterrs
          Control.packetsCorrupt += 1
          bitRatio = 1.0*biterrs/nbits
          byteRatio = 1.0*nByteErrors/nbytes
          logging.info("\n!!!! === DATA CORRUPT === !!!! %d/%d bits, %d/%d bytes wrong (%.2f, %.2f, r:%.2f) bytes %s\n" % (
            biterrs, nbits, nByteErrors, nbytes, bitRatio, byteRatio, byteRatio/bitRatio, byteErrors))
        else:
          logging.debug("-> %d bytes ok" % len(s))
      else:
        if s:
          sys.stdout.write(s)
        else:
          sys.stdout.write('!DATA CORRUPT!')
        sys.stdout.flush()
      Control.packetsReceived += 1
    #except BaseException, e:
    #  logging.info("%s" % e)

  def sendPacket(sender, data):
    assert len(data) <= 255, 'data length cannot exceed 255'
    hasher = md5.new()
    hasher.update(data)
    #packetHash = hasher.digest()[:3]
    packetHash = '';

    data = packetHash + chr(len(data)) + data
    logging.info('Packet len %d, num chunks %d' % (len(data), len(partition(data, CHUNK_DATA_BYTES))))

    chunks = []
    for chars in map(list, partition(data, CHUNK_DATA_BYTES)):
      if len(chars) < CHUNK_DATA_BYTES:
        pad(chars, '\0', CHUNK_DATA_BYTES)
      payload = ''.join(chars)
      chunks.append(payload)

    logging.debug("data: '%s'" % binascii.hexlify(data))
    preamble = genPreamble()
    final = list(preamble)

    logging.info('...packet')
    for chunk in chunks:
      chips = packeter.encodePacket(chunk)
      logging.info('...chunk')
      for chip in chips:
        logging.debug("chip: %s" % (chip))
        waveform = buildWaveform(chip, options.base, channelGap, chipSamples)

        waveform = waveform#[:-len(waveform) / 8]
        fadeout(waveform, int(chipSamples) / 10)
        fadein(waveform, int(chipSamples / 10))

        final.extend(waveform)

    sender.sendBlock(final)
    # Send a 1-chip silence buffer
    sender.sendBlock(silence(chipSamples))

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
    packetData = ''

    numChunkSymbols = packeter.symbolsForBytes(CHUNK_DATA_BYTES)
    remaining = -1
    logging.info('...receiving packet')
    while remaining != 0:
      chunkChips = []
      for symbolIndex in xrange(numChunkSymbols):
        waveform = receiver.receiveBlock(chipSamples)
        spectrum = fouriate(window(waveform))

        chip = []
        for f in xrange(options.base, options.base + (channelGap*options.numchans), channelGap):
          chip.append(spectrum.power(f))

        chunkChips.append(chip)
        
      (data, chips) = packeter.decodePacket(chunkChips)
      packetChips.extend(chips)
      logging.debug('')
      if data:
        logging.info("+++ Chunk received, SNR %.2f dB, corrected error rate %.3f" % 
            (assigner.lastSignalRatio(), packeter.lastErrorRate()))
        if remaining == -1:
          #packetHash = data[:3]
          #dataLength = ord(data[3])
          #chunkData = data[4:]
          #remaining = math.ceil(float(dataLength + 4) / CHUNK_DATA_BYTES)
          dataLength = ord(data[0])
          chunkData = data[1:]
          remaining = math.ceil(float(dataLength + 1) / CHUNK_DATA_BYTES)
          logging.info('Length %d' % dataLength)
        else:
          chunkData = data

      else:
        logging.info("--- Chunk DROPPED")
        return (None, packetChips)

      packetData += chunkData
      remaining -= 1
      logging.info('Chunks remaining: %d, Chunk data %s' % (remaining, chunkData))
      assert remaining >= 0

    packetData = packetData[:dataLength]
    return (packetData, packetChips)

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
    while Control.running: 
      time.sleep(0.3)
  except KeyboardInterrupt:
    pass
  except BaseException, e:
    logging.info("ERROR %s" % type(e))

  Control.running = False
  time.sleep(1)
  if options.csv:
    cols = map(str, (options.base, options.encoder, options.spacing, options.rate, options.numchans, options.redundancy, options.syncrate, options.numsyncchans))
    cols.extend(map(str, [Control.packetsSent, Control.packetsReceived, Control.packetsCorrupt, Control.packetsSent - Control.packetsReceived]))
    logging.info("=> " + ",".join(cols))
  else:
    if options.send:
      logging.info("=> %d packets sent" % Control.packetsSent)
    if options.listen:
      logging.info("=> %d packets received" % Control.packetsReceived)
      if options.test:
        logging.info("=> %d packets corrupt (%d bits)" % (Control.packetsCorrupt, Control.bitsCorrupt))
    if options.selftest:
      logging.info("=> %d packets dropped" % (Control.packetsSent - Control.packetsReceived))


def genTestData(nbytes):
  random.seed(2)
  return ''.join( (chr(random.randint(0, 255)) for i in xrange(nbytes)) )


if __name__ == "__main__":
  main()
