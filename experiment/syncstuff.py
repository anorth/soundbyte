import numpy as np
import math as m

def vectorCosineDetector(bucketVals, bitpattern):
  assert len(bucketVals) == len(bitpattern), '%s vs %s' % (bucketVals, bitpattern)
  bitpattern = bitpattern / np.linalg.norm(bitpattern)
  bucketVals = np.abs(bucketVals)
  norm = np.linalg.norm(bucketVals)
  bucketVals = bucketVals / norm
  result = np.dot(bucketVals, bitpattern)

# hack, apparently sometimes '1.0 <= 1.0' is false,
# haven't quite figured out why...
  result *= 0.99
  result += 0.005

  assert result >= 0 and result <= 1.0, result
  result = 1 - m.acos(result) * 4 / m.pi
  assert result >= -1 and result <= 1, result

  return result

def sinewave(cycles, count):
  return np.array([m.sin(x * 2 * m.pi * cycles / count)
    for x in xrange(count)])

def coswave(cycles, count):
  return np.array([m.cos(x * 2 * m.pi * cycles / count)
    for x in xrange(count)])

pattern = np.array([1,0,1,0,1,0,1,0])
opposite = np.array([1-b for b in pattern])

pattern2 = np.array([1,1,0,0,1,1,0,0])
opposite = np.array([1-b for b in pattern2])

pattern3 = np.array([1,1,1,1,0,0,0,0])
opposite = np.array([1-b for b in pattern2])

def combine(pattern, count, base=1):
  res = np.array([0.0] * count)
  for i in xrange(len(pattern)):
    if pattern[i]: res += sinewave(i+base, count)
  return res
    

def corr(signal, pattern, window, base):
  res = []

  l = len(signal) - window # max size to avoid overflow
  
  # reduce range
  for i in xrange(l - (l % (window*2))):
    spectrum = np.fft.rfft(signal[i:i+window])
    match = vectorCosineDetector(spectrum[base:base+len(pattern)], pattern)
    res.append(match)
  return res

