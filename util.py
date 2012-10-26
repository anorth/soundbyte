#!/usr/bin/env python

import math
import numpy as np
import scipy.misc as spm
import struct

class MovingAverage(object):
  def __init__(self, window):
    self.__avg = None
    self.__window = window

  def sample(self, v):
    if self.__avg is None:
      self.__avg = v
    else:
      self.__avg = ((self.__window - 1) * self.__avg + v) / self.__window

  def avg(self):
    return self.__avg

def flatten(listOfLists):
  # O(n) implementation
  return [item for sublist in l for item in sublist]

def countbits(v):
  c = 0;
  while v != 0:
    v &= v - 1; # clear the least significant bit set
    c += 1
  return c

# +/- 1.0
def signum(x):
  assert x != 0 # You're doing it wrong
  return math.copysign(1, x)

# Binomial coefficient, Cnk.
def comb(n, k):
  return int(spm.comb(n, k, True))

# The k-combination corresponding to integer i, as a sequence of k combinatorial
# elements [ck, ck-1, ... c1]. This is a unique sequence such that
#   i = C(ck, k) + C(ck-1, k-1) + ... + C(c1, 1)
# This is logically equivalent to the i'th n-bit number with k bits set,
# with the combinatorial elements being the indexes of the bits set.
def combinadic(k, i):
  elts = []
  kk = k
  n = i
  while n > 0:
    (el, cc) = maxCombElement(kk, n)
    elts.append(el)
    n -=  cc
    kk -= 1
  for j in xrange(len(elts), k):
    elts.append(k - j - 1)
  return elts

# The integer corresponding to a k-combination as produced by combinadic()
def inverseCombinadic(k, elts):
  i = 0
  kk = k
  for e in elts:
    i += comb(e, k)
    k -= 1
  return i

# Computes the maximum Cnk <= m
# Returns (n, Cnk)
def maxCombElement(k, m):
  cc = 0
  n = k - 1
  c = comb(n, k)
  while c <= m:
    cc = c
    n += 1
    c = comb(n, k)
  return (n - 1, cc)
