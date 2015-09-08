include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/client
  )

add_library(mirevdevutilsobjects OBJECT
    evdev_device_detection.cpp
    input_modifier_utils.cpp
    )

