set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(MIR_NDK_PATH $ENV{MIR_NDK_PATH} CACHE STRING "path of mir android bundle")

set(CMAKE_C_COMPILER   /usr/bin/arm-linux-gnueabihf-gcc-4.6)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-4.6)

# where too look to find dependencies in the target environment 
set(CMAKE_FIND_ROOT_PATH  "${MIR_NDK_PATH}")

#use only the cross compile system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#distinguish this from the bionic build
set (MIR_USES_BIONIC false CACHE BOOL "use the bionic libc/libstdc++")
