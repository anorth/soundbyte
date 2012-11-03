import logging
import numpy as np
import time
import math as m
import sys
from cmath import phase
from util import flatten

from plotter import Plot

from audio import *

PI2 = m.pi * 2

def genSync(base, numSyncChans, syncChannelGap, syncChipSamples):
  syncPairs = numSyncChans / 2
  pattern = [1,0] * syncPairs
  opposite = [1-b for b in pattern]

  # combine a decent chunk or we get a buffer underrun.
  def createSyncSignal(chipsPerPulse, repeats):
    p = pattern

    def createPulse(pattern):
      return list(buildWaveform(pattern, base, syncChannelGap, chipsPerPulse * syncChipSamples))
      return list(window(buildWaveform(pattern, base, syncChannelGap, chipsPerPulse * syncChipSamples)))

    p1 = createPulse(pattern)
    p2 = createPulse(opposite)

    return (p1 + p2) * repeats + p1

  chipsPerLongPulse = 2
  metaSignalBucket = 2
  syncLong = createSyncSignal(chipsPerLongPulse, metaSignalBucket + 1)
  syncReady = createSyncSignal(1, chipsPerLongPulse * 2) # * chipsPerLongPulse * metaSignalBucket

  #fadein(syncLong, chipSamples / 20)
  #fadeout(syncReady, chipSamples / 20)

  # silence is not actually necessary, it's just to prevent
  # buffer underruns with the audio output since we're not
  # sending any data here
  # FIXME this shouldn't be necessary
  #silenc = list(silence(chipSamples * 3))

  silenc = []
  silenc = list(silence(10000))

  #ttt =  time.time()
  #time.sleep(2)
 # for i in xrange(10)
  return np.array(silenc + syncLong + syncReady )
  #print time.time() - ttt


def vectorDetector(bucketVals, bitpattern):
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

  result = result * 2 - 1

  return result

class SyncUtil(object):
  def __init__(self, baseBucket, spacing, numchans,
      chipsPerSyncPulse=2, numCyclesAsReadyPulses=1,
      signalFactor=1.0,
      detectStrategy=vectorDetector,
      detectionSamplesPerChip = 4,
      misalignmentTolerance = 0.15,
      verbose = False
      ):
    """sync utility for sync signals of form SyncPulse ++ ChunkPulse, e.g.
    AAABBBAAABBBAAABBBABABABAB - the groups of 3 are the sync pulses, and then
    the groups of 1 are the "about to start data" pulses, of which there are
    a fixed number.

    signalFactor: arbitrary number as threshold of signal detection; zero treats
        everything as a signal, higher numbers are increasingly selective,
        infinity would be for a perfect, pure signal

    chipsPerSyncPulse: number of chips per "up" or "down" part of sync signal.
        e.g. if the sync signal is AAABBBAAABBB where each letter has duration
        of one chip,t hen chipsPerSyncPulse is 3.

    numCyclesAsReadyPulses: duration, in sync-pulse cycles (e.g. a full AAABBB)
        of the ABAB pulses in quick succession that occur to signal the start
        of the data section. E.g. if it is 1, and chipsPerSyncPulse is 3,
        then the 'ready' signal would be ABABAB (6 pulses) before data begins.

    detectionSamplesPerChip: number of times to apply the detection window
        per signal pulse. higher means more accurate but more CPU.

    misalignmentTolerance: proportion chipSize in misalignment that is tolerated
    """
    #assert chipsPerSyncPulse >= 2
    assert numCyclesAsReadyPulses == 1 # handling other values not implemented
    self.baseBucket = baseBucket
    self.spacing = spacing
    self.numchans = numchans
    self.chipsPerSyncPulse = chipsPerSyncPulse
    self.signalFactor = signalFactor
    self.detectStrategy = detectStrategy
    self.detectionSamplesPerChip = detectionSamplesPerChip
    self.misalignmentTolerance = misalignmentTolerance
    self.numCyclesAsReadyPulses = numCyclesAsReadyPulses
    self.verbose = verbose

    self.doPlot = True # todo: flag
    self.doPlot = False# True # todo: flag
    self.plotter = None

  def align(self, receiver, chipSize, patternUnit = [1,0]):
    """Listens for a sync signal from the receiver and aligns to it.

    Blocks, and when it returns, the receiver will be aligned to the start
    of the data to be read.
    """
    assert self.numchans % len(patternUnit) == 0
    pattern = np.array(patternUnit * (self.numchans / len(patternUnit)))
    assert len(pattern) == self.numchans

    pulseSize = self.chipsPerSyncPulse * chipSize
    pulseWindow = pulseSize

    signalCycleSize = 2 * pulseSize
    metaSignalBucket = 2 # TODO: fix code to avoid redundant work
    readyMetaSignalBucket = metaSignalBucket * self.chipsPerSyncPulse

    longSignalWindow = (1 + metaSignalBucket * 2) * pulseSize
    dataWindow = longSignalWindow + chipSize

    readySize = (1 + 2 * self.chipsPerSyncPulse) * chipSize
    assert readySize == signalCycleSize + chipSize

    metaSamplesPerPulse = self.chipsPerSyncPulse * self.detectionSamplesPerChip
    metaSamplesPerCycle = 2 * metaSamplesPerPulse
    samplesPerMetaSample = float(chipSize) / self.detectionSamplesPerChip

    assert self.chipsPerSyncPulse == (pulseWindow / chipSize)
    #bucketIndices = [
    #  self.chipsPerSyncPulse * (self.baseBucket + i * self.spacing)
    #  for i in xrange(self.numchans)]
    bucketIndices = [
      (self.baseBucket + i * self.spacing)
      for i in xrange(self.numchans)]

    data = list(receiver.receiveBlock(dataWindow))

    # -1: nothing
    #  0: got signal, alignment attempted for next cycle
    #  1: got signal and aligned
    #  2: expecting 'ready for data' cycle
    state = -1
    confidence2 = -1

    tries = 0
    oldMetaSignal = None

    while True:
      shortMetaSignal = []

      for i in xrange(longSignalWindow * self.detectionSamplesPerChip / chipSize):
        start = int(i * samplesPerMetaSample)
        end = start + chipSize # + pulseWindow
        assert end <= len(data)
        spectrum = np.fft.rfft(data[start:end])
        bucketVals = [spectrum[b] for b in bucketIndices]
        match = self.detectStrategy(bucketVals, pattern)
        shortMetaSignal.append(match)

      longMetaSignal = []
      for i in xrange(metaSamplesPerCycle * metaSignalBucket):
        avg = np.mean(shortMetaSignal[i:i+metaSamplesPerPulse])
        lms = avg
       # assert avg >= -1.0 and avg <= 1.0, avg
       # lms = 1 - m.acos(avg) * 2 / m.pi
        assert lms >= -1 and lms <= 1, lms
        longMetaSignal.append(lms)

      longMetaSpectrum = np.fft.rfft(longMetaSignal)
      partALength = metaSamplesPerCycle * (metaSignalBucket - 1)
      assert partALength >= metaSamplesPerCycle
      longMetaSpectrumA = np.fft.rfft(longMetaSignal[:partALength])
      longMetaSpectrumB = np.fft.rfft(longMetaSignal[partALength:])

      partBLength = self.chipsPerSyncPulse * (self.detectionSamplesPerChip * 2)
      #shortMetaSpectrumA = np.fft.rfft(shortMetaSignal[:-partBLength])
      shortMetaSpectrumB = np.fft.rfft(shortMetaSignal[-partBLength:])

      if self.doPlot:
        def fixSpectrum(d):
          #return abs(np.array([0] + list(d[1:])))
          return abs(d)
        spectrumPlotHeight = 10# 40
        plotData = [
          (-0.5, 0.5, data),
          (-1.0, 1.0, shortMetaSignal),
          (-1.0, 1.0, shortMetaSignal[-partBLength:]),
          (0, spectrumPlotHeight, fixSpectrum(shortMetaSpectrumB)),
          (-1.0, 1.0, longMetaSignal),
          (0, spectrumPlotHeight, fixSpectrum(longMetaSpectrum)),
          (-1.0, 1.0, longMetaSignal[:partALength]),
          (0, spectrumPlotHeight, fixSpectrum(longMetaSpectrumA)),
          (-1.0, 1.0, longMetaSignal[partALength:]),
          (0, spectrumPlotHeight, fixSpectrum(longMetaSpectrumB)),
          ##(0, spectrumPlotHeight, fixSpectrum(shortMetaSpectrumA)),
          ]
        if not self.plotter:
          self.plotter = Plot([(len(d), yMin, yMax) for (yMin, yMax, d) in plotData])
        for i, (_,_ , d) in enumerate(plotData):
          self.plotter.plot(i, d)

      def largestOther(spectrum, notBucket):
        return max([
            abs(spectrum[b]) for b in xrange(len(spectrum))
            if b != 0 and b != notBucket])

      def getAlignment(spectrum, bucket, state, chips, label, dbg=False):
        if not label: label = bucket == metaSignalBucket and '.' or '!' # for logging
        bucketValue = spectrum[bucket]
        resultAmplitude = abs(bucketValue)
        largestRemaining = largestOther(spectrum, bucket)
        #logging.debug('REMAINING: %s' % largestRemaining)
        misalignment = 0

        #logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))
        if (abs(bucketValue) > self.signalFactor * largestRemaining):
          #logging.debug('%s, %s' % (abs(bucketValue), largestRemaining))
          misalignment = phase(bucketValue) / PI2
          assert misalignment >= -0.5 and misalignment <= 0.5
          misalignmentPerChip = misalignment * chips
          if state == -1:
            logging.debug('[%s]' % label)
            result = 0 # don't consider misalignment on first go
          elif abs(misalignmentPerChip) <= self.misalignmentTolerance:
            result = 1
            if dbg: 
              logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))
            logging.debug('(%s %.3f)' % (label, abs(misalignmentPerChip)))
          else:
            #logging.debug('<%s>' % label, False)
            logging.debug('<%s:%s>' % (label, misalignmentPerChip))
            #logging.debug('MISALIGNED %s %.2f' % (label, misalignmentPerChip))
            result = 0 # couldn't have been a good signal
        else:
          result = -1
          logging.debug('%s' % label)
          if dbg: 
            logging.debug('%s:%s' % (phase(bucketValue), abs(bucketValue)))

        return (result, misalignment)


      #if state == 2:
      #  #logging.debug(metaSignal)
      #  #if newState == -1:
      #  #  tries += 1
      #  #  if tries < 3:
      #  #    newState = 1
      #  #  else:
      #  #    tries = 0

      #  # check for, and verify alignment to, the ready signal
      #  #logging.debug('A')
      #  (result, dummy) = getAlignment(shortMetaSpectrum,
      #      readyMetaSignalBucket, 1, 2*chipSize, True)
      #  #print 'THIS: ', readyMetaSignalBucket
      #  if result == 1:
      #    # Got ready signal, aligned and ready!
      #    logging.debug('ALIGNED')
      #    return

      #  # Fail. start again.
      #  state = -1
      #  #import time
      #  #time.sleep(10)

      if state == 2:
        (resultShortB, dummy) = getAlignment(shortMetaSpectrumB,
            self.chipsPerSyncPulse, 1, 1, '#')
        if resultShortB == 1:
          logging.debug('ALIGNED')
          #time.sleep(5)
          #receiver.receiveBlock(int(chipSize*2.0))
          #receiver.receiveBlock(int(chipSize*0.0))
          logging.info("aligned")
          return
        else:
          #time.sleep(10)
          state = -1

      if state == 1:
        (resultLongA, dummy) = getAlignment(longMetaSpectrumA,
            metaSignalBucket - 1, 1, self.chipsPerSyncPulse, '.')
        (resultLongB, dummy) = getAlignment(longMetaSpectrumB,
            1, 1, self.chipsPerSyncPulse, '.')
        (resultShortB, chipMisalignment) = getAlignment(shortMetaSpectrumB,
            self.chipsPerSyncPulse, 1, 1, '#')
        logging.debug('\n|| %s %s %s' % (resultLongA, resultLongB, resultShortB))
        if resultLongA == 1 and resultLongB < 1 and resultShortB >= 0:
          logging.debug('========= %s %s %s' % (resultLongA, resultLongB, resultShortB))
          state = 2
          misalignment = chipMisalignment / self.chipsPerSyncPulse
          #time.sleep(10)
          #logging.debug('ALIGNED')
          #return

      if state < 2:
        # Check for, and align to, the long signal
        #logging.debug('L')
        (state, misalignment) = getAlignment(longMetaSpectrum,
            metaSignalBucket, state, self.chipsPerSyncPulse, '.')

      if state < 0:
        misalignment = 0

      if True or state < 2:
        alignedAmount = int(signalCycleSize * (1 - misalignment))
      else:
        alignedAmount = signalCycleSize * (metaSignalBucket - 1) + pulseSize
        logging.debug('aligning +%d cycles' % (metaSignalBucket - 1))

      data = data[alignedAmount:] + list(receiver.receiveBlock(alignedAmount))

