#!/bin/sh

if [ "$1" ]; then d="-s $1"; fi

adb $d forward tcp:16000 tcp:16000
adb $d forward tcp:16001 tcp:16001
