# By default, build for LLVM

include_directories(${GLESv2_INCLUDE_DIRS})

if (MIR_PLATFORM STREQUAL "android")
  include_directories(${LIBHARDWARE_INCLUDE_DIRS})
  add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(gbm/)
endif()

set(
  GRAPHICS_SOURCES

  gl_renderer.cpp
  display_configuration.cpp
  null_display_report.cpp
)

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(egl/)
endif()

add_library(
  mirgraphics STATIC

  ${GRAPHICS_SOURCES}
)

target_link_libraries(
  mirgraphics

  mirlogging

  ${GLESv2_LIBRARIES}
)
