#!/usr/bin/env bash

set -e
set -x

uname -sr

git submodule init
git submodule update

mkdir build
cd build
cmake ${CMAKE_FLAGS:-} ..
cmake --build .
cmake --build . -- check
printf "find-me says: "
./test/find-me/find-me

if [ "$(uname -s)" = "Darwin" ]; then
  SUDO=sudo
else
  # sudo not needed
  SUDO=
fi
${SUDO} cmake --build . -- install
