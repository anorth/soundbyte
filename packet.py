#!/usr/bin/env python

import itertools
import logging
import math
import numpy as np
import struct

from util import *

from reedsolomon import Codec, UncorrectableError

class DecodeException(BaseException):
  pass

# Interface Packeter
# - encodePacket(data):
# Builds a payload, encoding a data string into a sequence
# of chips (each a sequence of 1/0)
#
# - decodePacket(chips):
# Decodes a packet of chips into data.
# 
# - symbolsForBytes(nBytes)
# Returns number of symbols to transmit nbytes of data
#
# - lastErrorRate():
# Bit error rate of the last signal decoded.
class Packeter(object):
  def __init__(self, encoder, assigner):
    self.encoder = encoder
    self.assigner = assigner

  def encodePacket(self, data):
    encoded = self.encoder.encode(data)
    return self.assigner.encodeChips(encoded)

  def decodePacket(self, chips):
    encoded = self.assigner.decodeChips(chips)
    return ''.join(self.encoder.decode(encoded))

  def symbolsForBytes(self, nbytes):
    return self.assigner.symbolsForBits(self.encoder.encodedBitsForBytes(nbytes))

  def bitsPerChip(self):
    return float(self.assigner.bitsPerChip()) / (self.encoder.encodedBitsForBytes(100) / 800)

  def lastErrorRate(self):
    return self.encoder.lastErrorRate()

# Interface Encoder
# - encode(bytes):
# Transforms raw bytes to a list of encoded bits
#
# - decode(bitLikelihoods):
# Transforms encoded bit likelihoods to raw data, or None
# 
# - encodedBitsForBytes(nBytes):
# The number of bits needed to encode n data bits
class IdentityEncoder(object):
  def encode(self, data):
    # TODO: recursive systematic convolution to distribute bits?
    # TODO: add ECC, LDPC?
    return toBitSequence(data)

  # Decode
  def decode(self, bits):
     # TODO see encode
    return toByteSequence(bits)

  def encodedBitsForBytes(self, nbytes):
    return nbytes * 8

  def lastErrorRate(self):
    return 0.0

class ReedSolomonEncoder(object):

  def __init__(self, width, encodedSize, messageSymbols):
    logging.info('ENCODER %s %s' % (encodedSize, messageSymbols))
    assert encodedSize > messageSymbols
    self.width = width
    self.messageSymbols = int(messageSymbols)
    self.encodedSize = int(encodedSize)
    self.c = Codec(self.encodedSize, self.messageSymbols)

  def encode(self, data):
    #logging.info("VAG %s ", data)
    #data = list(data)
    #logging.info("DICK")
    #data = ''.join(data)
    #logging.info("COCK %s" % data)
    assert len(data) == self.messageSymbols # todo: relax this & fix encodedBitsForBytes
    #logging.info("ZING")
    encoded = self.c.encode(data)
    #logging.info("ASDF")
    #return toBitSequence([encoded[i] for i in xrange(len(encoded))])
    return list(toBitSequence(encoded))

  def decode(self, signals):
    cropped = ( ''.join(toByteSequence(signals + ([0] * (8 - len(signals) % 8)))) )[:self.encodedSize]
    try:
      return self.c.decode(cropped)[0]
    except UncorrectableError, e:
      raise DecodeException()
      
    
  def encodedBitsForBytes(self, nbytes):
    return self.encodedSize * nbytes * 8 / self.messageSymbols

  def lastErrorRate(self):
    return 0.0
    

# Encoder which repeats width-length chunks of the data N times and 
# takes a majority vote when decoding
class RepeatingEncoder(object):
  def __init__(self, n, width):
    assert n % 2 != 0, "Majority encoder requires odd redundancy"
    self.n = int(n)
    self.width = int(width)
    # Last bit error rate
    self.err = 0.0
    assert self.n > 0

  def encode(self, data):
    bits = list(toBitSequence(data))
    encoded = []
    for chunk in partition(bits, self.width):
      pad(chunk, 0, self.width) # pad final chunk
      encoded.extend(chunk * self.n)
    return encoded

  def decode(self, signals):
    assert len(signals) % self.width == 0
    assert len(signals) % self.n == 0
    agreement = []
    decodedLength = len(signals) / self.n
    signals = map(signum, signals)
    decoded = []
    errors = 0
    chunks = partition(signals, self.width)
    for chunkList in partition(chunks, self.n):
      counts = [ sum(b) for b in zip(*chunkList) ]
      #print counts
      errors += sum([ self.n - abs(c) for c in counts ]) / 2
      bits = map(signum, counts)
      decoded.extend(bits)
    # Compute errors
    self.err = float(errors) / (len(signals))
    for (decision, cs) in zip(partition(decoded, self.width), partition(chunks, self.n)):
      c = []
      for cc in cs:
        es = [cc[i] == decision[i] and 1 or 0 for i in xrange(len(decision))]
        c.append(es)
      agreement.append(c)
    #print "agreement"
    #print '\n'.join(map(str, agreement))

    #print "chips"
    #print '\n'.join(map(str, chunks))

    # Strip trailing non-byte signals
    if len(decoded) % 8:
      decoded = decoded[:-(len(decoded)%8)]
    return toByteSequence(decoded)

  def encodedBitsForBytes(self, nbytes):
    bits = nbytes * 8
    pad = 0
    if bits % self.width:
      pad = self.width - bits % self.width
    return (bits + pad) * self.n

  def lastErrorRate(self):
    return self.err


# Like RepeatingEncoder but interleaves the repetitions
class InterleavingRepeatingEncoder(RepeatingEncoder):

  def encode(self, data):
    # Encode with the raw repeating encoder
    bits = RepeatingEncoder.encode(self, data)
    # Interleave repeats
    chips = partition(bits, self.width)
    symbolsRepeated = partition(chips, self.n)
    interleaved = []
    for symbolsOnce in zip(*symbolsRepeated):
      map(lambda s: interleaved.extend(s), symbolsOnce)
    #logging.info(str(interleaved))
    return interleaved

  def decode(self, signals):
    # De-interleave repeated signals
    chips = partition(signals, self.width)
    nSymbols = len(chips) / self.n
    repeats = partition(chips, nSymbols)
    deinterleaved = []
    for message in zip(*repeats):
      map(lambda s: deinterleaved.extend(s), message)
    #logging.info(deinterleaved)

    # Decode with raw repeating decoder
    return RepeatingEncoder.decode(self, deinterleaved)


# Interface CarrierAssigner
# - encode(bits):
# Transforms a sequence of bits (0/1) into a sequence of 
# chips, each a sequence of 1s or 0s. 
# The final chip may be padded with zeros.
#
# - decode(chips):
# Transforms a sequence of chips (lists of power values for channels)
# into a bit sequence, with probabilities [-1.0..1.0]
#
# - symbolsForBits(nbits):
# The number of symbols required to transmit n bits
#
# - bitsPerChip():
# The number of whole bits in a chip

# Assigns each bit to two channels, setting one high:
# 0 => (0, 1)
# 1 => (1, 0)
class PairwiseAssigner(object):
  def __init__(self, nchans):
    self.nchans = nchans

  def encodeChips(self, bitstring):
    chips = []
    currentchip = []
    for bit in bitstring:
      if bit > 0:
        currentchip.extend([1, 0])
      else:
        currentchip.extend([0, 1])
      if len(currentchip) >= self.nchans - 1:
        pad(currentchip, 0, self.nchans)
        chips.append(currentchip)
        currentchip = []
    if len(currentchip):
      pad(currentchip, 0, self.nchans)
      assert len(currentchip) == self.nchans
      chips.append(currentchip)
    return chips

  def decodeChips(self, chips):
    bits = [] # list of reals
    for c in chips:
      assert len(c) == self.nchans
      for pair in partition(c, 2):
        if len(pair) == 2:
          bits.append(pair[0] > pair[1] and 1.0 or -1.0)
    return bits

  def symbolsForBits(self, nbits):
    return nbits / (self.nchans / 2)

  def bitsPerChip(self):
    return self.nchans / 2


# Encodes chips of data as one of C(nchans, nchans/2) combinations of
# nchans, half on and half off.
# TODO: generalise to more k != nchans/2
class CombinadicAssigner(object):
  def __init__(self, nchans):
    assert nchans % 2 == 0
    self.nchans = nchans
    self.width = int(math.log(comb(nchans, nchans / 2)) / math.log(2))
    self.lastSnr = 0

  def encodeChips(self, bitstring):
    chips = []
    k = self.nchans / 2
    #print bitstring
    for bits in partition(bitstring, self.width):
      i = toInt(bits)
      signalIndexes = combinadic(k, i)
      #print signalIndexes
      currentchip = [0] * self.nchans
      for si in signalIndexes:
        assert si < len(currentchip)
        currentchip[si] = 1.0
      #print currentchip
      assert sum(currentchip) == k
      chips.append(currentchip)
    return chips

  def decodeChips(self, chips):
    bits = [] # list of reals
    k = self.nchans / 2
    # SNR for each chip. Each chip's SNR is ratio of lowest "on" channel
    # to highest "off" channel
    snrs = []
    for chip in chips:
      assert len(chip) == self.nchans
      # Partition signals into high half and low half
      signals = [ (signal, i) for (i, signal) in enumerate(chip) ]
      signals.sort(reverse=True)
      (high, low) = partition(signals, len(signals)/2)
      indexes = [ i for (signal, i) in high ]
      snrs.append(dbPower(high[-1][0], low[0][0]))
      indexes.sort(reverse=True)
      #print indexes
      assert len(indexes) == k
      n = inverseCombinadic(k, indexes)
      bits.extend(toBits(n, self.width))
    #print bits
    self.lastSnr = sum(snrs) / len(snrs) # Average of chip SNRs
    return bits

  def symbolsForBits(self, nbits):
    return int(math.ceil(1.0 * nbits / self.width))

  def bitsPerChip(self):
    return self.width

  def lastSignalRatio(self):
    return self.lastSnr


# A generator that yields one bit at a time from a string of bytes,
# each byte least significant first.
def toBitSequence(bytesequence):
  for b in (ord(c) for c in bytesequence):
    for i in xrange(8):
      yield (b & (1 << i)) >> i
      
# A generator that yields bytes (as 1-char strings) from a sequence of bits
# (or bit likelihoods). Each byte is expected least-significant-bit first.
def toByteSequence(bitsequence):
  assert len(bitsequence) % 8 == 0, "invalid bitsequence len %d" % len(bitsequence)
  for byte in partition(bitsequence, 8):
    yield chr(toInt(byte))

# Converts a sequence of bits (or probabilities), least significant first, into an integer
def toInt(bits):
  v = 0
  i = 0
  for bit in bits:
    if bit > 0: v |= 1 << i
    i += 1
  return v

# Converts an integer to a bit probability sequence (all +/- 1.0)
def toBits(i, nbits):
  return [ (i & (1 << j)) >> j and 1.0 or -1.0 for j in xrange(nbits) ]
