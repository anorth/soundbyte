#!/bin/sh
#
# Runs run.py once as sender and once as listener, connected directly by pipe.
# Any command-line arguments are passed to both processes; -i and -s/l will be passed
# automatically.
#
# Stdin will be input to the sender and stdout will be output from the receiver.
# 
# E.g. `sh sendreceive.sh -t` to run self-test

./run.py -s -i $* | ./run.py -l -i $*
