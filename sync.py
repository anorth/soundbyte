import numpy as np
import math as m
import sys
from cmath import phase
from util import flatten

from plotter import Plot

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
  sys.stdout.write(str(text))
  if newline: sys.stdout.write('\n')
  sys.stdout.flush()

class SyncUtil(object):
  def __init__(self, baseBucket, spacing, numchans,
      chunksPerSyncPulse=3, numCyclesAsReadyPulses=1,
      signalFactor=1.25,
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

    numCyclesAsReadyPulses: duration, in sync-pulse cycles (e.g. a full AAABBB)
        of the ABAB pulses in quick succession that occur to signal the start
        of the data section. E.g. if it is 1, and chunksPerSyncPulse is 3,
        then the 'ready' signal would be ABABAB (6 pulses) before data begins.

    detectionSamplesPerPulse: number of times to apply the detection window
        per signal pulse. higher means more accurate but more CPU.

    misalignmentTolerance: proportion chunkSize in misalignment that is tolerated
    """
    #assert chunksPerSyncPulse >= 2
    assert numCyclesAsReadyPulses == 1 # handling other values not implemented
    self.baseBucket = baseBucket
    self.spacing = spacing
    self.numchans = numchans
    self.chunksPerSyncPulse = chunksPerSyncPulse
    self.signalFactor = signalFactor
    self.detectStrategy = detectStrategy
    self.detectionSamplesPerPulse = detectionSamplesPerPulse
    self.misalignmentTolerance = misalignmentTolerance
    self.numCyclesAsReadyPulses = numCyclesAsReadyPulses

    self.plotter = None


  def align(self, receiver, chunkSize, patternUnit = [1,0,1,0]):
    """Listens for a sync signal from the receiver and aligns to it.

    Blocks, and when it returns, the receiver will be aligned to the start
    of the data to be read.
    """
    assert self.numchans % len(patternUnit) == 0
    pattern = np.array(patternUnit * (self.numchans / len(patternUnit)))
    assert len(pattern) == self.numchans

    pulseSize = self.chunksPerSyncPulse * chunkSize
    pulseWindow = pulseSize

    signalCycleSize = 2 * pulseSize
    metaSignalBucket = 2 # TODO: fix code to avoid redundant work
    readyMetaSignalBucket = metaSignalBucket * self.chunksPerSyncPulse

    signalWindow = (1 + metaSignalBucket * 2) * pulseSize

    metaSamplesPerCycle = 2 * self.detectionSamplesPerPulse
    samplesPerMetaSample = float(signalCycleSize) / metaSamplesPerCycle

    assert self.chunksPerSyncPulse == (pulseWindow / chunkSize)
    bucketIndices = [
      self.chunksPerSyncPulse * (self.baseBucket + i * self.spacing)
      for i in xrange(self.numchans)]

    data = list(receiver.receiveBlock(signalWindow))

    # -1: nothing
    #  0: got signal, alignment attempted for next cycle
    #  1: got signal and aligned
    #  2: expecting 'ready for data' cycle
    state = -1
    confidence2 = -1

    tries = 0
    oldMetaSignal = None
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
      metaSpectrumA = np.fft.rfft(metaSignal[:-metaSamplesPerCycle])
      metaSpectrumB = np.fft.rfft(metaSignal[-metaSamplesPerCycle:])

      spectrumPlotSize = len(metaSpectrum)
      #if not self.plotter:
      #  self.plotter = Plot([
      #      (len(data), -0.5, 0.5),
      #      (len(metaSignal), -1.0, 1.0),
      #      (spectrumPlotSize, 0, 50)
      #      ])
      #self.plotter.plot(0, data)
      #self.plotter.plot(1, metaSignal)
      #self.plotter.plot(2, abs(np.array([0] + list(metaSpectrum[1:spectrumPlotSize]))))

      def largestOther(spectrum, notBucket):
        return max([
            abs(spectrum[b]) for b in xrange(len(spectrum))
            if b != 0 and b != notBucket])

      def getAlignment(spectrum, bucket, state, cycleSize, dbg=False):
        label = bucket == metaSignalBucket and '.' or '!' # for logging
        bucketValue = spectrum[bucket]
        resultAmplitude = abs(bucketValue)
        largestRemaining = largestOther(spectrum, bucket)
        #log('REMAINING: %s' % largestRemaining)
        misalignment = 0

        #log('%s:%s' % (phase(bucketValue), abs(bucketValue)))
        if (abs(bucketValue) > self.signalFactor * largestRemaining):
          #log('%s, %s' % (abs(bucketValue), largestRemaining))
          misalignment = phase(bucketValue) / PI2
          misalignmentPerChunk = misalignment * cycleSize / chunkSize
          if state == -1:
            log('[%s]' % label, False)
            result = 0 # don't consider misalignment on first go
          elif abs(misalignmentPerChunk) <= self.misalignmentTolerance:
            result = 1
            if dbg: 
              log('%s:%s' % (phase(bucketValue), abs(bucketValue)))
            #log('Aligned%s %.3f' % (label, abs(misalignmentPerChunk)))
          else:
            log('<%s>' % label, False)
            #log('MISALIGNED %s %.2f' % (label, misalignmentPerChunk))
            result = 0 # couldn't have been a good signal
        else:
          result = -1
          log(label, False)
          if dbg: 
            log('%s:%s' % (phase(bucketValue), abs(bucketValue)))

        return (result, misalignment)


      if state == 2:
        #log(metaSignal)
        #if newState == -1:
        #  tries += 1
        #  if tries < 3:
        #    newState = 1
        #  else:
        #    tries = 0

        # check for, and verify alignment to, the ready signal
        #log('A')
        (result, dummy) = getAlignment(metaSpectrum,
            readyMetaSignalBucket, 1, 2*chunkSize, True)
        #print 'THIS: ', readyMetaSignalBucket
        if result == 1:
          # Got ready signal, aligned and ready!
          return

        # Fail. start again.
        state = -1
        #import time
        #time.sleep(10)


      if state == 1:
        (result1, dummy) = getAlignment(metaSpectrumA,
            metaSignalBucket - 1, 1, signalCycleSize)
        (result2, dummy) = getAlignment(metaSpectrumB,
            1, 1, signalCycleSize)
        #(result2, dummy) = getAlignment(metaSpectrumB,
        #    self.chunksPerSyncPulse, 1, 2*chunkSize)
        if result1 == 1 and result2 == -1:
          state = 2


      if state < 2:
        # Check for, and align to, the long signal
        #log('L')
        (state, misalignment) = getAlignment(metaSpectrum,
            metaSignalBucket, state, signalCycleSize)

      if state < 0:
        misalignment = 0

      if state < 2:
        alignedAmount = int(signalCycleSize * (1 - misalignment))
      else:
        alignedAmount = signalCycleSize * (metaSignalBucket - 1)
        print 'aligning +%d cycles' % (metaSignalBucket - 1)

      data = data[alignedAmount:] + list(receiver.receiveBlock(alignedAmount))

