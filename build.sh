#!/bin/sh

set -ex
mkdir -p m4
autoreconf -i
rm -rf build
mkdir build
cd build
../configure \
  --enable-silent-rules \
  CFLAGS="-O -ggdb -Wall"

make check VERBOSE=t
