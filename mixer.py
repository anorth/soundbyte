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
  parser.add_option('-s', '--signal', type='int', default="-10",
      help='Signal strength, dB below unity')
  parser.add_option('-n', '--noise', type='int', default="0",
      help='Noise strength, dB below unity')

  return parser.parse_args()
  
def main():
  (options, args) = parseArgs()
  instream = StreamReceiver(sys.stdin)
  outstream = StreamSender(sys.stdout)
  if options.play:
    playstream = PyAudioSender()
  noiseGain = dbAmplitudeGain(options.noise)
  signalGain = dbAmplitudeGain(options.signal)
  noise = None
  noiseOffset = 0
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
        if noiseOffset + len(block) > len(noise):
          noiseOffset = 0
        blockNoise = noise[noiseOffset:noiseOffset + len(block)]
        noiseOffset += len(block)
        block = sum([block, blockNoise])
      limit(block)
      outstream.sendBlock(block)
      if options.play:
        playstream.sendBlock(block)
  except KeyboardInterrupt, e:
    pass


if __name__ == "__main__":
  main()
