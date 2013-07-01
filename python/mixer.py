#!/usr/bin/env python

from optparse import OptionParser
import sys
import wave

from audio import *
from util import *

SAMPLES_PER_BLOCK = SAMPLE_RATE / 100

def parseArgs():
  parser = OptionParser()
  parser.add_option('-p', '--play', action='store_true')
  parser.add_option('-f', '--file', type='str',
      help='WAV file path')
  parser.add_option('-s', '--signal', type='int', default="-6",
      help='Signal strength, dB below unity')
  parser.add_option('-n', '--noise', type='int', default="-6",
      help='Noise strength, dB below unity')

  return parser.parse_args()

# Receiver decorator which mixes received signal with a noise file.
class NoiseMixingReceiver(object):
  def __init__(self, receiver, filename, signalGainDb, noiseGainDb):
    self.receiver = receiver
    self.signalGain = dbAmplitudeGain(signalGainDb)
    self.noiseGain = dbAmplitudeGain(noiseGainDb)
    self.noise = None
    self.noiseOffset = 0
    if filename:
      wf = wave.open(filename, 'rb')
      assert wf.getframerate() == SAMPLE_RATE, "Unnacceptable sample rate %d" % wf.getframerate()
      assert wf.getsampwidth() == 2, wf.getsampwidth()
      noise = decodePcm(wf.readframes(wf.getnframes()))
      self.noise = noise * self.noiseGain

  def receiveBlock(self, numSamples):
    block = self.receiver.receiveBlock(numSamples)
    if self.noise != None:
      block = block * self.signalGain
      if self.noiseOffset + len(block) > len(self.noise):
        self.noiseOffset = 0
      blockNoise = self.noise[self.noiseOffset:self.noiseOffset + len(block)]
      self.noiseOffset += len(block)
      block = sum([block, blockNoise])
      limit(block)
    return block
  
def main():
  (options, args) = parseArgs()
  outstream = StreamSender(sys.stdout)
  instream = NoiseMixingReceiver(StreamReceiver(sys.stdin), options.file, options.signal, options.noise)
  if options.play:
    instream = PlaybackReceiver(instream)
  try:
    while True:
      block = instream.receiveBlock(SAMPLES_PER_BLOCK)
      outstream.sendBlock(block)
  except KeyboardInterrupt, e:
    pass


if __name__ == "__main__":
  main()
