#!/bin/bash

# --base
# --spacing
# --rate
# --syncrate
# --redundancy
# --numchans
# --numsyncchans
# --encoder


BASE="15000 13000"
ENCODER="repeat rs lay"
SPACING="2 3"
RATE="100 50 25"
NUMCHANS="16 12 8"

NOISE=$1
SIGNAL=$2

for base in $BASE; do
  for encoder in $ENCODER; do
    for spacing in $SPACING; do
      for rate in $RATE; do
        for numchans in $NUMCHANS; do
          echo "base=$base encoder=$encoder spacing=$spacing rate=$rate numchans=$numchans"
          ./run.py --selftest -f $NOISE --signal $SIGNAL --testpackets 20 --base $base --encoder $encoder --spacing $spacing --rate $rate --numchans $numchans 2> >(grep '^=>')
        done
      done
    done
  done
done
