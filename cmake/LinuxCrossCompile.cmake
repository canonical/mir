set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(MIR_NDK_PATH $ENV{MIR_NDK_PATH} CACHE STRING "path of mir android bundle")

set(CMAKE_C_COMPILER   /usr/bin/${TARGET_ARCH}-gcc${CC_VARIANT})
set(CMAKE_CXX_COMPILER /usr/bin/${TARGET_ARCH}-g++${CC_VARIANT})

# where to look to find dependencies in the target environment
set(CMAKE_FIND_ROOT_PATH  "${MIR_NDK_PATH}")

#treat the chroot's includes as system includes
include_directories(SYSTEM "${MIR_NDK_PATH}/usr/include" "${MIR_NDK_PATH}/usr/include/${TARGET_ARCH}")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "${MIR_NDK_PATH}/usr/include" "${MIR_NDK_PATH}/usr/include/${TARGET_ARCH}" )

# Add the chroot libraries as system libraries
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
  "${MIR_NDK_PATH}/lib"
  "${MIR_NDK_PATH}/lib/${TARGET_ARCH}"
  "${MIR_NDK_PATH}/usr/lib"
  "${MIR_NDK_PATH}/usr/lib/${TARGET_ARCH}"
)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_EXECUTABLE_RUNTIME_C_FLAG "-Wl,-rpath-link,")
set(CMAKE_EXECUTABLE_RUNTIME_CXX_FLAG "-Wl,-rpath-link,")
set(CMAKE_INSTALL_RPATH "${MIR_NDK_PATH}/lib:${MIR_NDK_PATH}/lib/${TARGET_ARCH}:${MIR_NDK_PATH}/usr/lib:${MIR_NDK_PATH}/usr/lib/${TARGET_ARCH}")

set(ENV{PKG_CONFIG_PATH} "${MIR_NDK_PATH}/usr/lib/pkgconfig:${MIR_NDK_PATH}/usr/lib/${TARGET_ARCH}/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${MIR_NDK_PATH}")

#use only the cross compile system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
