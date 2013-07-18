#!/bin/sh
#
# Builds distribution zip file for Android.
# Requires that the project has been built in Eclipse first.

# Halt on error
set -e

JAR="bin/scom.jar"
JARDST="libs"

LIB="obj/local/armeabi/libscomjni.so"
LIBDST="libs/armeabi"

MK="Android.mk.example"
MKDST="jni"

TARGET="`pwd`/soundbyte-android.zip"

tmpdir=`mktemp -d /tmp/scomdist-XXXX`
if [ ! -d ${tmpdir} ]
then
  echo "Failed to make temporary directory for distribution"
  exit 1
fi
rm -f ${TARGET}

mkdir -p ${tmpdir}/${JARDST}
cp ${JAR} ${tmpdir}/${JARDST}

mkdir -p ${tmpdir}/${LIBDST}
cp ${LIB} ${tmpdir}/${LIBDST}

mkdir -p ${tmpdir}/${MKDST}
cp ${MK} ${tmpdir}/${MKDST}/Android.mk

cd ${tmpdir}
zip -r --no-dir-entries ${TARGET} *
cd

rm -r ${tmpdir}
unzip -t ${TARGET}
