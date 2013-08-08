import csv
import numpy as np
from audio import *

def slurp_csv(name):
  """
  slurps the given 1 column csv file into a numpy array
  """
  with open(name, 'rb') as csvfile:
    r = csv.reader(csvfile)
    return np.array([float(row[0]) for row in r])

def pcmfile_reader(name):
  return StreamReceiver(open(name, 'rb'))

def pcmfile_writer(name):
  return StreamSender(open(name, 'wb'))

def slurp_pcm(name):
  return slurp_stream(pcmfile_reader(name))

def slurp_stream(receiver):
  sound = []
  with receiver:
    while True:
      b = list(receiver.receiveBlock(10000))
      sound += b

      if (len(b) < 10000): break

  return sound

def write_pcmfile(array, tofile):
  """
  Normalizes the given array or list, encodes to pcm, and dumps to the file
  """
  with StreamSender(open(tofile, 'wb')) as out:
    out.sendBlock(normalise(np.array(array)))

class ListReceiver(object):
  def __init__(self, data):
    self.data = list(data)

  def receiveBlock(self, numSamples):
    if (numSamples > len(self.data)):
      res = self.data
      self.data = []
    else:
      res = self.data[:numSamples]
      self.data = self.data[numSamples:]
    return np.array(res)


  def __enter__(self):
    return self
        
  def __exit__(self, type_, value, traceback):
    pass

  

def convolve_file(ir, input_name, output_name):
  with StreamSender(open(output_name, 'wb')) as outs:
    convolve_stream(ir, StreamReceiver(open(input_name, 'rb')), outs)

def convolve_stream(ir, receiver, sender):
  ir = np.array(ir)
  """
  convolves the input stream with the given impulse-response

  closes the input stream when there is nothing left in it. leaves the output stream open
  """

  with receiver:
    missed = len(ir)
    blocksize = 10 * 44100 + 2*missed
    prev = []
    while True:
      received = list(receiver.receiveBlock(blocksize))
      b = prev + received
      if len(b) < missed:
        break

      prev = b[len(b) - missed:]

      # TODO: something better than normalizing each block separately
      sender.sendBlock(normalise(np.convolve(ir, b, mode='valid')))
      if len(received) < blocksize:
        break

def cat_stream(receiver, sender):
  with receiver:
    blocksize = 50000
    while True:
      b = list(receiver.receiveBlock(blocksize))
      sender.sendBlock(b)
      if len(b) < blocksize:
        break

class PortionReceiver(object):
  def __init__(self, receiver, max_samples):
    self.receiver = receiver
    self.remaining = max_samples

  def __enter__(self):
    return self
        
  def __exit__(self, type_, value, traceback):
    pass

  def receiveBlock(self, numSamples):
    if (numSamples > self.remaining): numSamples = self.remaining
    self.remaining -= numSamples

    return self.receiver.receiveBlock(numSamples)

spk = PyAudioSender()
mic = PyAudioReceiver()

def mike_receiver(numSamples):
  return PortionReceiver(mic, numSamples)

def mike_samples(numSamples):
  return mic.receiveBlock(numSamples)

def play(soundwave):
  spk.sendBlock(soundwave)

def playn(soundwave):
  spk.sendBlock(normalise(np.array(soundwave)))
