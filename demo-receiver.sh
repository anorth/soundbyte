#!/bin/sh
nc localhost 16000 | ./run.py -i -l $* | nc localhost 16001
