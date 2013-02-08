#!/bin/bash
#
# Usage: echo "message" | demo-sender.sh

./run.py -s -i $* | nc localhost 16001 > /dev/null
