set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(MIR_NDK_PATH $ENV{MIR_NDK_PATH} CACHE STRING "path of mir android bundle")

if (NOT DEFINED MIR_TARGET_MACHINE)
  set(MIR_TARGET_MACHINE $ENV{MIR_TARGET_MACHINE} CACHE STRING "target machine")
endif()
if (NOT DEFINED MIR_GCC_VARIANT)
  set(MIR_GCC_VARIANT $ENV{MIR_GCC_VARIANT} CACHE STRING "gcc variant required")
endif()

set(CMAKE_C_COMPILER   /usr/bin/${MIR_TARGET_MACHINE}-gcc${MIR_GCC_VARIANT})
set(CMAKE_CXX_COMPILER /usr/bin/${MIR_TARGET_MACHINE}-g++${MIR_GCC_VARIANT})

# where to look to find dependencies in the target environment
set(CMAKE_FIND_ROOT_PATH  "${MIR_NDK_PATH}")

#treat the chroot's includes as system includes
include_directories(SYSTEM "${MIR_NDK_PATH}/usr/include" "${MIR_NDK_PATH}/usr/include/${MIR_TARGET_MACHINE}")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "${MIR_NDK_PATH}/usr/include" "${MIR_NDK_PATH}/usr/include/${MIR_TARGET_MACHINE}" )

# Add the chroot libraries as system libraries
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
  "${MIR_NDK_PATH}/lib"
  "${MIR_NDK_PATH}/lib/${MIR_TARGET_MACHINE}"
  "${MIR_NDK_PATH}/usr/lib"
  "${MIR_NDK_PATH}/usr/lib/${MIR_TARGET_MACHINE}"
)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_EXECUTABLE_RUNTIME_C_FLAG "-Wl,-rpath-link,")
set(CMAKE_EXECUTABLE_RUNTIME_CXX_FLAG "-Wl,-rpath-link,")
set(CMAKE_INSTALL_RPATH "${MIR_NDK_PATH}/lib:${MIR_NDK_PATH}/lib/${MIR_TARGET_MACHINE}:${MIR_NDK_PATH}/usr/lib:${MIR_NDK_PATH}/usr/lib/${MIR_TARGET_MACHINE}")

set(ENV{PKG_CONFIG_PATH} "${MIR_NDK_PATH}/usr/lib/pkgconfig:${MIR_NDK_PATH}/usr/lib/${MIR_TARGET_MACHINE}/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${MIR_NDK_PATH}")

#use only the cross compile system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
