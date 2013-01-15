set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(MIR_NDK_PATH $ENV{MIR_NDK_PATH} CACHE STRING "path of mir android bundle")

set(CMAKE_C_COMPILER   /usr/bin/arm-linux-gnueabihf-gcc-4.6)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-4.6)

# where too look to find dependencies in the target environment 
set(CMAKE_FIND_ROOT_PATH  "${MIR_NDK_PATH}")

include_directories(SYSTEM ${MIR_NDK_PATH}/usr/include)
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "${MIR_NDK_PATH}/usr/include")

#use only the cross compile system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
