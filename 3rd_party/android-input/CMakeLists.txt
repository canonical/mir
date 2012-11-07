# Don't be so strict on warnings etc as android code won't compile with them on.
# A *big* number of warnings would ensue. Therefore redefine CMAKE_CXX_FLAGS without
# things such as -Werror, -Wall, etc
set(CMAKE_CXX_FLAGS "-fno-strict-aliasing --std=c++0x")

# A bad hack (repeating this) but it fixes the build for CI
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

# FIXME: Do proper platform detection here.
set(ANDROID_INPUT_COMPILE_FLAGS
    "-include ${CMAKE_CURRENT_SOURCE_DIR}/android/system/core/include/arch/ubuntu-x86/AndroidConfig.h")

set(MIR_ANDROID_INPUT_COMPILE_FLAGS
    "${ANDROID_INPUT_COMPILE_FLAGS} -DUSING_STD_CPP11"
    PARENT_SCOPE)

add_subdirectory(android)
add_subdirectory(tools)

# Make those include directories available to mir source code
list(
  APPEND MIR_ANDROID_INCLUDE_DIRECTORIES
  # That's what mir is really interested in
  ${CMAKE_CURRENT_SOURCE_DIR}/android/frameworks/base/services/input
  # And those are referenced by the headers in the directory above
  ${CMAKE_CURRENT_SOURCE_DIR}/android/frameworks/base/include
  ${CMAKE_CURRENT_SOURCE_DIR}/android/frameworks/native/include
  ${CMAKE_CURRENT_SOURCE_DIR}/android/hardware/libhardware_legacy/include
  ${CMAKE_CURRENT_SOURCE_DIR}/android/system/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/android/system/extras/ext4_utils
  )
set(MIR_ANDROID_INCLUDE_DIRECTORIES ${MIR_ANDROID_INCLUDE_DIRECTORIES} PARENT_SCOPE)
