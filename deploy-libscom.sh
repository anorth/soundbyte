#!/bin/sh
SCOMJAR="Scom/bin/scom.jar"
SCOMLIB="Scom/obj/local/armeabi/libscomjni.so"

SCOMJAR_DST="SoundbyteListener/libs/"
SCOMLIB_DST="SoundbyteListener/libs/armeabi/"
ANDROIDMK_DST="SoundbyteListener/jni/"

set -x

if [ ! -f ${SCOMJAR} ]
then
  echo "Can't find ${SCOMJAR}. Rebuild the Scom project"
  exit 1
fi

if [ ! -d ${SCOMJAR_DST} ]
then
  mkdir ${SCOMJAR_DST}
fi

if [ ! -d ${SCOMLIB_DST} ]
then
  mkdir ${SCOMLIB_DST}
fi

if [ ! -d ${ANDROIDMK_DST} ]
then
  mkdir ${ANDROIDMK_DST}
fi

cp ${SCOMJAR} ${SCOMJAR_DST}
cp ${SCOMLIB} ${SCOMLIB_DST}

cat > ${ANDROIDMK_DST}/Android.mk <<EOF
LOCAL_PATH := \$(call my-dir)
include \$(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES += scomjni
\$(call import-module,native)
EOF

echo "Done"

