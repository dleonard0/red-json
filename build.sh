#!/bin/sh

set -ex
mkdir -p m4
autoreconf -v -i
rm -rf build
mkdir build
cd build
../configure \
  --enable-silent-rules \
  CFLAGS="-O -ggdb -Wall"

make check V=1 VERBOSE=t
