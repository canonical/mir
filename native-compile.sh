#!/bin/sh
# Native build script for Mir
#
# Builds the project with gcc-4.4, as specified by the toolchain file
set -e

CMAKE_TOOLCHAIN_FILE=cmake/Toolchain-Ubuntu-GCC44.cmake

if [ ! -e build-native ]; then
    mkdir build-native
    ( cd build-native && cmake -DCMAKE_TOOLCHAIN_FILE=../$CMAKE_TOOLCHAIN_FILE ..)
fi

cmake --build build-native
