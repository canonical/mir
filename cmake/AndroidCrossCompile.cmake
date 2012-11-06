set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(MIR_NDK_PATH $ENV{MIR_NDK_PATH} CACHE STRING "path of mir android ndk")

set(CMAKE_C_COMPILER   ${MIR_NDK_PATH}/bin/arm-linux-androideabi-gcc)
set(CMAKE_CXX_COMPILER ${MIR_NDK_PATH}/bin/arm-linux-androideabi-g++)

# where too look to find dependencies in the target environment 
set(CMAKE_FIND_ROOT_PATH  ${MIR_NDK_PATH}/sysroot)

#use only the cross compile system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

