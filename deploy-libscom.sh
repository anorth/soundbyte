#!/bin/sh
#
# Packages Scom distribution and unzips it into a target directory, which should be an
# Android project root.

# Halt on error
set -e

TARGET=$1
if [ ! -d "${TARGET}" ]
then
  echo "Usage: $0 app-directory"
  exit 1
fi

cd Scom
./dist.sh
sdk="`pwd`/soundbyte-android.zip"
cd -

cd ${TARGET}
unzip -o ${sdk}

echo "Done"

