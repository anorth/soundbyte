#!/bin/sh

set -e
make -j 40 && bin/stuff -t && (
  if [ "$1" = "-d" ]; then
    bin/stuff -s | bin/stuff -l 
  else
    bin/stuff -s 2>/dev/null | bin/stuff -l 2>/dev/null
  fi
)
