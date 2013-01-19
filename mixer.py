#!/usr/bin/env python

from optparse import OptionParser
import sys
import wave

from audio import *
from util import *

SAMPLES_PER_BLOCK = SAMPLE_RATE / 100

def parseArgs():
  parser = OptionParser()
  parser.add_option('-l', '--listen', action='store_true')
  parser.add_option('-f', '--file', type='str',
      help='WAV file path')
  parser.add_option('-s', '--signal', type='int', default="-10",
      help='Signal strength, dB below unity')
  parser.add_option('-n', '--noise', type='int', default="-10",
      help='Noise strength, dB below unity')

  return parser.parse_args()
  
def main():
  (options, args) = parseArgs()
  instream = StreamReceiver(sys.stdin)
  outstream = StreamSender(sys.stdout)
  noiseGain = dbAmplitudeGain(options.noise)
  signalGain = dbAmplitudeGain(options.signal)
  noise = None
  noiseoffset = 0
  if options.file:
    wf = wave.open(options.file, 'rb')
    assert wf.getframerate() == SAMPLE_RATE, "Unnacceptable sample rate %d" % wf.getframerate()
    assert wf.getsampwidth() == 2, wf.getsampwidth()
    noise = decodePcm(wf.readframes(wf.getnframes()))
    noise = noise * noiseGain
  try:
    while True:
      block = instream.receiveBlock(SAMPLES_PER_BLOCK)
      block = block * signalGain
      if noise != None:
        if noiseoffset + len(block) > len(noise):
          noiseoffset = 0
        blockNoise = noise[noiseoffset:noiseoffset + len(block)]
        block = sum([block, blockNoise])
        #logging.error("%d %f %f" % (len(blockNoise), max(blockNoise), max(block)))

      outstream.sendBlock(block)
  except KeyboardInterrupt, e:
    pass


if __name__ == "__main__":
  main()
