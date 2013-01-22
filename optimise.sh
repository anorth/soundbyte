#!/bin/bash

# --base
# --encoder
# --spacing
# --rate
# --redundancy
# --numchans
# --syncrate
# --numsyncchans


BASE="15000"
ENCODER="repeat rs"
SPACING="2 3 4"
RATE="100 50 25"
NUMCHANS="16 12 8 4"
REDUNDANCY="5 3"

NOISE=$1
SIGNAL=$2

for base in $BASE; do
  for encoder in $ENCODER; do
    for spacing in $SPACING; do
      for rate in $RATE; do
        for numchans in $NUMCHANS; do
          echo "base=$base encoder=$encoder spacing=$spacing rate=$rate numchans=$numchans"
          ./run.py --selftest --csv -f $NOISE --signal $SIGNAL --repeat 20 --base $base --encoder $encoder --spacing $spacing --rate $rate --numchans $numchans 2> >(grep '^=>')
        done
      done
    done
  done
done
