# Don't be so strict on warnings etc as android code won't compile with them on.
# A *big* number of warnings would ensue. Therefore locally define CMAKE_CXX_FLAGS
# without things such as -Werror, -Wall, etc
STRING(REGEX REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
STRING(REGEX REPLACE "-W[^ ]*" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

set(ANDROID_INPUT_COMPILE_FLAGS
    "-include ${CMAKE_CURRENT_SOURCE_DIR}/android/system/core/include/arch/ubuntu-x86/AndroidConfig.h -Wno-format")

set(MIR_ANDROID_INPUT_COMPILE_FLAGS
    "${ANDROID_INPUT_COMPILE_FLAGS} -DUSING_STD_CPP11"
    PARENT_SCOPE)

add_subdirectory(android)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/android/external/kernel-headers/original
  )
set(MIR_ANDROID_INCLUDE_DIRECTORIES ${MIR_ANDROID_INCLUDE_DIRECTORIES} PARENT_SCOPE)
