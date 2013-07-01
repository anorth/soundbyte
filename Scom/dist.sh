#!/bin/sh

JAR="bin/scom.jar"
LIB="obj/local/armeabi/libscomjni.so"
MK="Android.mk.example"
TARGET="`pwd`/soundbyte-android.zip"

tmpdir=`mktemp -d /tmp/scomdist-XXXX`
if [ ! -d ${tmpdir} ]
then
  echo "Failed to make temporary directory for distribution"
  exit 1
fi
rm -f ${TARGET}

mkdir -p `dirname ${tmpdir}/${JAR}`
cp ${JAR} ${tmpdir}/${JAR}

mkdir -p `dirname ${tmpdir}/${LIB}`
cp ${LIB} ${tmpdir}/${LIB}

mkdir -p ${tmpdir}/jni
cp ${MK} ${tmpdir}/jni/Android.mk

cd ${tmpdir}
zip -r --no-dir-entries ${TARGET} *
cd

rm -r ${tmpdir}
unzip -t ${TARGET}
