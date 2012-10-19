#!/usr/bin/env python

import itertools
import math
import numpy as np
import struct


# Builds a packet payload, encoding a string of bytes into a sequence
# of chips (sequence of 1/0), each nChannels wide.
def makePacketPayload(data, nChannels):
  encoded = encodeBits(data)
  return PairwiseAssigner().encodeChips(toBits(data), nChannels)

# Encodes data into a packet (as a byte string)
def encodeBits(data):
  # TODO: recursive systematic convolution to distribute bits?
  # TODO: add ECC, LDPC?
  return data

# Interface CarrierAssigner
# - encode(bits, nchans):
# Transforms a sequence of bits (0/1) into a sequence of 
# chips, each a sequence of 
# nChans 1s or 0s. The final chip may be padded with zeros.
#
# - decode(chips):
# Transforms a sequence of chips (lists of power values for channels)
# into a bit sequence, with probabilities [-1.0..1.0]

# Assigns each bit to two channels, setting one high:
# 0 => (0, 1)
# 1 => (1, 0)
class PairwiseAssigner(object):
  def encodeChips(self, bitstring, nChans):
    chips = []
    currentchip = []
    for bit in bitstring:
      if bit > 0:
        currentchip.extend([1, 0])
      else:
        currentchip.extend([0, 1])
      if len(currentchip) >= nChans - 1:
        pad(currentchip, 0, nChans)
        chips.append(currentchip)
        currentchip = []
    if len(currentchip):
      pad(currentchip, 0, nChans)
      chips.append(currentchip)
    return chips

  def decodeChips(self, chips):
    bits = [] # list of reals
    for c in chips:
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
def toBytes(bitsequence):
  for byte in partition(bitsequence, 8):
    b = 0
    for i in xrange(8):
      if byte[i] > 0: b |= 1 << i
    yield chr(b)


print PairwiseAssigner().encodeChips(toBits("A"), 16)
print PairwiseAssigner().decodeChips(PairwiseAssigner().encodeChips(toBits("A"), 16))
print list(toBytes(PairwiseAssigner().decodeChips(PairwiseAssigner().encodeChips(toBits("A"), 16))))


