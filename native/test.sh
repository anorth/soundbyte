#!/bin/sh

set -e
make -j 40 && bin/stuff -s 2>/dev/null | bin/stuff -l 2>/dev/null
