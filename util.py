#!/usr/bin/env python

import math
import numpy as np
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
