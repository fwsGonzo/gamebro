#!/bin/bash
set -e
mkdir -p build
pushd build
cmake ..
make -j3
popd
./build/gamebro $1
