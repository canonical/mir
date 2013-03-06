include_directories(
  ../android/external/kernel-headers/original
  ../android/frameworks/base/include
  ../android/frameworks/base/services/input
  ../android/frameworks/native/include
  ../android/hardware/libhardware_legacy/include
  ../android/system/core/include
  ../android/system/extras/ext4_utils
  /usr/include
  )

###### raw-event-printer

add_executable(
  raw-event-printer
  raw_event_printer.cpp
  )

set_target_properties(
  raw-event-printer
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
  )

if(MIR_PLATFORM STREQUAL "gbm")
  target_link_libraries(
    raw-event-printer

    android-input
    )
endif()

if(MIR_PLATFORM STREQUAL "android")
  target_link_libraries(
    raw-event-printer

    android-input
    )
endif()

###### cooked-event-printer

add_executable(
  cooked-event-printer
  cooked_event_printer.cpp
  )

set_target_properties(
  cooked-event-printer
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
  )

if(MIR_PLATFORM STREQUAL "gbm")
  target_link_libraries(
    cooked-event-printer
    android-input
    )
endif()

if(MIR_PLATFORM STREQUAL "android")
  target_link_libraries(
    cooked-event-printer
    android-input
    )
endif()

###### dispatcher-check

add_executable(
  dispatcher-check
  dispatcher_check.cpp
  )

set_target_properties(
  dispatcher-check
  PROPERTIES
  COMPILE_FLAGS ${ANDROID_INPUT_COMPILE_FLAGS}
  )

if(MIR_PLATFORM STREQUAL "gbm")
  target_link_libraries(
    dispatcher-check
    android-input
    -lrt
    -lpthread
    )
endif()

if(MIR_PLATFORM STREQUAL "android")
  target_link_libraries(
    dispatcher-check
    android-input
    )
endif()


if (NOT MIR_INPUT_USE_ANDROID_TYPES)
  target_link_libraries(cooked-event-printer    ${Boost_LIBRARIES})
  target_link_libraries(raw-event-printer       ${Boost_LIBRARIES})
  target_link_libraries(dispatcher-check        ${Boost_LIBRARIES})
endif()

