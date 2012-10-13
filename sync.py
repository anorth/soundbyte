import numpy as np

# Detection strategy:
#
# returns number between 0..1 for strength of 
# double detect(spectrum, expectedOn, expectedOff)

def vectorCosineDetector(bucketVals, bitpattern):
  bitpattern /= np.linalg.norm(bitpattern)
  bucketVals /= np.linalg.norm(np.abs(bucketVals))
  result = np.dot(bucketVals, bitpattern)
  assert result >= 0 and result <= 1

  return result


class Util(object):
  def __init__(self, baseBucket, gap, numchans,
      signalToBackgroundRatio=2.0,
      readAheadChunks=4
      detectStrategy=vectorCosineDetector
      ):
    self.baseBucket = baseBucket
    self.gap = gap
    self.numchans = numchans
    self.signalFactor = signalToBackgroundRatio
    self.numReadAhead = 4

    self.buckets = [baseBucket + b * (1 + gap) for b in range(0, numchans)]

#  def onOffBuckets(self, pattern):
#    assert len(pattern) == self.numchans
#
#    bucket = self.baseBucket
#    on  = []
#    off = []
#    for bit in pattern:
#      assert bit == 0 or bit == 1
#      if bit == 1:
#        on.append(bucket)
#      elif bit == 0:
#        off.append(bucket)
#      else:
#        raise Exception('bits must be 1 or 0')
#      bucket += self.gap + 1
#
#    return (on, off)
#


  def align(self, receiver, chunkSize, pattern):
    pattern = np.array(pattern)
    opposite = -pattern

    # XXX XXX
    # applying the detector to a sliding window should produce
    # another sine wave, so we can do a correlation with 
    # e ^ i.k.x  on that to get the strength of match for
    # the alignment match strength and use the argument for the phase
    # so we know where to align to.

    data = []
    for i in range(0, self.numReadAhead):
      data += receiver.receiveBlock(chunkSize)



    spectrum = np.fft.rfft(data)
    for 

    
  
    



    

    
    
    

