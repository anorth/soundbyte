#!/bin/sh
#
# Removes files installed in an Android project by deploy-libscom.sh

TARGET=$1
if [ ! -d "${TARGET}" ]
then
  echo "Usage: $0 app-directory"
  exit 1
fi

rm -rf ${TARGET}/libs/armeabi/ ${TARGET}/libs/scom.jar ${TARGET}/jni/Android.mk
