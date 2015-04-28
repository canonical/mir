include_directories(
  external/kernel-headers/original
  frameworks/base/include
  frameworks/native/include
  hardware/libhardware_legacy/include
  system/core/include
  system/extras/ext4_utils

  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
)

add_definitions(
  -DANDROID_SMP=1
  )

# This is stuff for which we want non-android versions
set(
  UTIL_SOURCES

  frameworks/native/libs/utils/Looper.cpp     # used by InputDispatcher.cpp
  frameworks/native/libs/utils/RefBase.cpp    # used a lot
  frameworks/native/libs/utils/Timers.cpp     # used by KeyCharacterMap.cpp, InputReader.cpp, InputDispatcher.cpp, Looper.cpp
)


add_library(
  android-input-static STATIC
  # The stuff that we want
  frameworks/base/services/input/EventHub.cpp
  frameworks/base/services/input/InputApplication.cpp
  frameworks/base/services/input/InputDispatcher.cpp
  frameworks/base/services/input/InputListener.cpp
  frameworks/base/services/input/InputReader.cpp
  frameworks/base/services/input/InputTransport.cpp
  frameworks/base/services/input/InputWindow.cpp
  frameworks/base/services/input/IntSet.cpp
  frameworks/base/services/input/PointerController.cpp
  # Keyboard/keymap handling
  frameworks/base/services/input/GenericKeyMap.cpp
  # The stuff that is used internally by the implementation above.
  # We're not directly interested in those:
  frameworks/base/services/input/Input.cpp
  frameworks/base/services/input/InputDevice.cpp
  frameworks/base/services/input/Keyboard.cpp
  frameworks/base/services/input/KeyCharacterMap.cpp
  frameworks/base/services/input/KeyLayoutMap.cpp
  frameworks/base/services/input/VelocityControl.cpp
  frameworks/base/services/input/VelocityTracker.cpp
  frameworks/base/services/input/VirtualKeyMap.cpp
  frameworks/base/services/input/MirLog.cpp
  frameworks/native/libs/utils/Tokenizer.cpp
  frameworks/native/libs/utils/FileMap.cpp
  ${UTIL_SOURCES}
  )

#we don't have binder or skia available on hybris/linux platforms
if(MIR_USE_BIONIC)
  target_link_libraries(
    android-input-static
    binder
    skia)
endif()

target_link_libraries(
  android-input-static

  ${Boost_LIBRARIES}
)

set_target_properties(
  android-input-static
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
)
