import numpy as np
import math as m
import sys
from cmath import phase
from util import flatten

PI2 = m.pi * 2

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

def logret(val, text='%s'):
  print text % val
  return val

def log(text='', newline=True):
  sys.stdout.write(text)
  if newline: sys.stdout.write('\n')
  sys.stdout.flush()

class SyncUtil(object):
  def __init__(self, baseBucket, spacing, numchans,
      chunksPerSyncPulse=2, numChunkPulses=6,
      signalFactor=0.2,
      detectStrategy=vectorCosineDetector,
      detectionSamplesPerPulse = 20,
      misalignmentTolerance = 0.2
      ):
    """sync utility for sync signals of form SyncPulse ++ ChunkPulse, e.g.
    AAABBBAAABBBAAABBBABABABAB - the groups of 3 are the sync pulses, and then
    the groups of 1 are the "about to start data" pulses, of which there are
    a fixed number.

    signalFactor: arbitrary number as threshold of signal detection; zero treats
        everything as a signal, higher numbers are increasingly selective,
        infinity would be for a perfect, pure signal

    chunksPerSyncPulse: number of chunks per "up" or "down" part of sync signal.
        e.g. if the sync signal is AAABBBAAABBB where each letter has duration
        of one chunk,t hen chunksPerSyncPulse is 3.

    numChunkPulses: number of ABAB pulses in quick succession (ABAB would be 4).

    detectionSamplesPerPulse: number of times to apply the detection window
        per signal pulse. higher means more accurate but more CPU.

    misalignmentTolerance: proportion chunkSize in misalignment that is tolerated
    """
    self.baseBucket = baseBucket
    self.spacing = spacing
    self.numchans = numchans
    self.chunksPerSyncPulse = chunksPerSyncPulse
    self.signalFactor = signalFactor
    self.detectStrategy = detectStrategy
    self.detectionSamplesPerPulse = detectionSamplesPerPulse
    self.misalignmentTolerance = misalignmentTolerance


  def align(self, receiver, chunkSize, patternUnit = [1,0,1,0]):
    """Listens for a sync signal from the receiver and aligns to it.
    Because it reads ahead, it returns the start of the data section
    following the sync signal that it read ahead into. The size of the
    data section read into will be aligned to chunkSize, so subsequent
    reads from receiver can simply be of size chunkSize.

    NOTE: XXX XXX currently doesn't actually do this. just loops forever
    printing out signal detection and alignment information.
    """
    assert self.numchans % len(patternUnit) == 0
    pattern = np.array(patternUnit * (self.numchans / len(patternUnit)))
    assert len(pattern) == self.numchans

    pulseSize = self.chunksPerSyncPulse * chunkSize
    pulseWindow = pulseSize

    signalCycleSize = 2 * pulseSize
    metaSignalBucket = 4 # TODO: fix code to avoid redundant work

    signalWindow = (1 + metaSignalBucket * 2) * pulseSize

    metaSamplesPerCycle = 2 * self.detectionSamplesPerPulse
    samplesPerMetaSample = float(signalCycleSize) / metaSamplesPerCycle

    assert self.chunksPerSyncPulse == (pulseWindow / chunkSize)
    bucketIndices = [
      self.chunksPerSyncPulse * (self.baseBucket + i * self.spacing)
      for i in xrange(self.numchans)]

    data = list(receiver.receiveBlock(signalWindow))

    confidence = -1

    while True:
      metaSignal = []

      #print signalWindow / chunkSize, len(data), samplesPerMetaSample, chunkSize
      for i in xrange(metaSamplesPerCycle * metaSignalBucket + 1):
        start = int(i * samplesPerMetaSample)
        end = start + pulseWindow
        #print 'Loop %s (%s:%s - %s/%s)' %(i, start, end, end-start, len(data))
        #print i, metaSamplesPerCycle * metaSignalBucket
        assert end <= len(data)
        spectrum = np.fft.rfft(data[start:end])
        bucketVals = [spectrum[b] for b in bucketIndices]
        match = self.detectStrategy(bucketVals, pattern)
        metaSignal.append(match)

      metaSpectrum = np.fft.rfft(metaSignal)
      result = metaSpectrum[metaSignalBucket]

      resultAmplitude = abs(result)
      sumRemainingAmplitudes = sum([
          abs(metaSpectrum[b]) for b in xrange(len(metaSpectrum))
          if b != 0 and b != metaSignalBucket])


      #log('\n\n========================')
      #print metaSignal
      #log('')
      #log(['%.2f' % np.abs(v) for v in metaSpectrum[:8]])
      misalignment = 0
      if (abs(result) > self.signalFactor * sumRemainingAmplitudes):
        misalignment = phase(result) / PI2
        misalignmentPerChunk = misalignment * signalCycleSize / chunkSize
        if confidence == -1:
          log('*', False)
          confidence = 0 # don't consider misalignment on first go
        elif abs(misalignmentPerChunk) <= self.misalignmentTolerance:
          log('Aligned %.3f %s' % (abs(misalignmentPerChunk), confidence))
          #log(' %s ' % confidence, False)
          confidence += 1
        else:
          #log('!', False)
          log('MISALIGNED %.2f' % (misalignmentPerChunk))
          confidence = 0 # couldn't have been a good signal
      else:
        confidence = -1
        #log('No sync signal (%.1f vs %.1f)' % (resultAmplitude, sumRemainingAmplitudes))
        log('.', False)

      alignedAmount = int(signalCycleSize * (1 - misalignment))
      data = data[alignedAmount:] + list(receiver.receiveBlock(alignedAmount))

