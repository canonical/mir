include_directories(
  external/kernel-headers/original
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

add_library(android-input-static STATIC
  frameworks/base/services/input/EventHub.cpp
  frameworks/base/services/input/InputDevice.cpp
  frameworks/base/services/input/InputListener.cpp
  frameworks/base/services/input/InputReader.cpp
  frameworks/base/services/input/InputTransport.cpp
  frameworks/base/services/input/Input.cpp
  frameworks/base/services/input/IntSet.cpp
  frameworks/base/services/input/GenericKeyMap.cpp
  frameworks/base/services/input/Keyboard.cpp
  frameworks/base/services/input/KeyCharacterMap.cpp
  frameworks/base/services/input/KeyLayoutMap.cpp
  frameworks/base/services/input/MirLog.cpp
  frameworks/native/libs/utils/Tokenizer.cpp
  frameworks/base/services/input/VelocityControl.cpp
  frameworks/base/services/input/VelocityTracker.cpp
  frameworks/base/services/input/VirtualKeyMap.cpp
  frameworks/native/libs/utils/FileMap.cpp
  frameworks/native/libs/utils/RefBase.cpp    # used a lot
  frameworks/native/libs/utils/Timers.cpp     # used by KeyCharacterMap.cpp, InputReader.cpp, InputDispatcher.cpp, Looper.cpp
)

target_link_libraries(
  android-input-static

  ${Boost_LIBRARIES}
)

set_target_properties(
  android-input-static
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
)
