#!/usr/bin/env bash

set -e
set -x

uname -sr

git submodule init
git submodule update

mkdir build
cd build
cmake ..
cmake --build .
cmake --build . -- check
printf "find-me says: "
./test/find-me/find-me 
cmake --build . -- install
