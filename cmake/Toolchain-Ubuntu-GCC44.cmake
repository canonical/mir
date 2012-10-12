# Sample toolchain file for building for ARM from an Ubuntu Linux system.
#
# Typical usage:
#    *) install gcc 4.4: sudo apt-get install gcc-4.4 g++-4.4
#    *) cd build
#    *) cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain-Ubuntu-GCC44.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(TOOLCHAIN_PREFIX )
set(TOOLCHAIN_SUFFIX -4.4)

# Native but non-default compilers 
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_SUFFIX})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_SUFFIX})
