#!/bin/bash


function on_exit() {
    rm -f FIFO
}

trap on_exit EXIT

if [ ! -e FIFO ]; then mkfifo FIFO; fi

while true; do nc localhost 16000 < FIFO; done \
  | ./run.py -i -l $* > FIFO
