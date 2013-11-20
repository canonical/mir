include_directories(${GLESv2_INCLUDE_DIRS})

add_library(
  mirgraphics STATIC

  default_configuration.cpp
  default_display_configuration_policy.cpp
)

add_subdirectory(nested/)
add_subdirectory(offscreen/)

if (MIR_PLATFORM STREQUAL "android")
  include_directories(${LIBHARDWARE_INCLUDE_DIRS})
  add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(gbm/)
endif()

