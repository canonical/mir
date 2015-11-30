include_directories(
  external/kernel-headers
  frameworks/base/include
  frameworks/native/include
  hardware/libhardware_legacy/include
  system/core/include
  system/extras/ext4_utils

  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
)

add_definitions(
  -DANDROID_SMP=1
  )

add_library(android-input-common OBJECT
  frameworks/base/services/input/InputTransport.cpp
  frameworks/base/services/input/Input.cpp
  frameworks/base/services/input/IntSet.cpp
  frameworks/base/services/input/MirLog.cpp
  frameworks/native/libs/utils/RefBase.cpp    # used a lot
  frameworks/native/libs/utils/Timers.cpp     # used by KeyCharacterMap.cpp, InputReader.cpp, InputDispatcher.cpp, Looper.cpp
)

set_target_properties(
  android-input-common
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
)
