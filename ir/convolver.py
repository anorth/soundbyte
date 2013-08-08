#!/usr/bin/env python

import sys                                                                                                       
from audio import *
from util import *
import numpy as np

if len(sys.argv) < 2:
  print 'usage: %s ir.csv < signaldata > convolveddata' % sys.argv[0]
  sys.exit(1)


ir = normalise(np.array(slurp_csv(sys.argv[1])))
sender = StreamSender(sys.stdout)
receiver = StreamReceiver(sys.stdin)

convolve_stream(ir, receiver, sender)

