#!/usr/bin/env python

import itertools
import math
import numpy as np
import struct


# Interface Packeter
# - encodePacket(data):
# Builds a payload, encoding a data string into a sequence
# of chips (each a sequence of 1/0)
#
# - decodePacket(chips):
# Decodes a packet of chips into data.
class Packeter(object):
  def __init__(self, encoder, assigner):
    self.encoder = encoder
    self.assigner = assigner

  def encodePacket(self, data):
    encoded = self.encoder.encode(data)
    return self.assigner.encodeChips(toBits(data))

  def decodePacket(self, chips):
    encoded = self.assigner.decodeChips(chips)
    return ''.join(toBytes(self.encoder.decode(encoded)))

# Builds a packet payload, encoding a string of bytes into a sequence
# of chips (sequence of 1/0), each nChannels wide.
#def encodePacketPayload(data, nChannels):
#  encoded = IdentityEncoder().encode(data)
#  return PairwiseAssigner().encodeChips(toBits(data), nChannels)

#def decodePacketPayload(chips):
#  encoded = PairwiseAssigner().decodeChips(chips)
#  return ''.join(toBytes(IdentityEncoder().decode(encoded)))


# Interface Encoder
# - encode(bytes):
# Transforms raw bytes to (possibly more) encoded bytes
#
# decode(bytes):
# Transforms encoded byts to raw bytes, or None
class IdentityEncoder(object):
  def encode(self, data):
    # TODO: recursive systematic convolution to distribute bits?
    # TODO: add ECC, LDPC?
    return data

  # Decode
  def decode(self, data):
     # TODO see encode
    return data

# Interface CarrierAssigner
# - encode(bits):
# Transforms a sequence of bits (0/1) into a sequence of 
# chips, each a sequence of 1s or 0s. 
# The final chip may be padded with zeros.
#
# - decode(chips):
# Transforms a sequence of chips (lists of power values for channels)
# into a bit sequence, with probabilities [-1.0..1.0]

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
      chips.append(currentchip)
    return chips

  def decodeChips(self, chips):
    bits = [] # list of reals
    for c in chips:
      assert len(c) == self.nchans
      for pair in partition(c, 2):
        bits.append(pair[0] > pair[1] and 1 or 0)
    return bits


# Partitions a sequence into subsequences of length n
def partition(sequence, n):
  return [sequence[i:i+n] for i in xrange(0, len(sequence), n)]

# Pads a list with value to some total length
def pad(lst, value, tolen):
  lst.extend([value] * (len(lst) - tolen))
 
# A generator that yields one bit at a time from a string of bytes
def toBits(bytesequence):
  for b in (ord(c) for c in bytesequence):
    for i in xrange(8):
      yield (b & (1 << i)) >> i
      
# A generator that yields bytes (as 1-char strings) from a sequence of bits
# (or bit likelihoods)
def toBytes(bitsequence):
  assert len(bitsequence) % 8 == 0, "invalid bitsequence " + str(bitsequence)
  for byte in partition(bitsequence, 8):
    b = 0
    for i in xrange(8):
      if byte[i] > 0: b |= 1 << i
    yield chr(b)


