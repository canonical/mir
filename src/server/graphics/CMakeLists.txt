# By default, build for LLVM

include_directories(${GLESv2_INCLUDE_DIRS})

add_subdirectory(nested/)

if (MIR_PLATFORM STREQUAL "android")
  include_directories(${LIBHARDWARE_INCLUDE_DIRS})
  add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(gbm/)
endif()

set(
  GRAPHICS_SOURCES

  egl_extensions.cpp
  egl_resources.cpp
  display_configuration.cpp
  null_display_report.cpp
  default_display_configuration_policy.cpp
  buffer_basic.cpp
)

add_library(
  mirgraphics STATIC

  ${GRAPHICS_SOURCES}
)

target_link_libraries(
  mirgraphics

  mirlogging

  ${MIR_COMMON_PLATFORM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
