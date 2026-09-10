#!/usr/bin/env bash

set -e
set -x

uname -sr
which bash
which python3
python3 --version

git submodule init
git submodule update

cmake ${CMAKE_FLAGS:-} -B build -S .
cmake --build build
cmake --build build --target check
printf "find-me says: "
./build/test/find-me/find-me

sudo cmake --install build
