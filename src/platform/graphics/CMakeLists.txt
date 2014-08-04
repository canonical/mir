# Workaround for bug 1208774 (spurious link errors on 32bit g++)
string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
include_directories(${GLESv2_INCLUDE_DIRS})

set(
  GRAPHICS_SOURCES

  egl_extensions.cpp
  egl_resources.cpp
  display_configuration.cpp
  buffer_basic.cpp
  pixel_format_utils.cpp
  overlapping_output_grouping.cpp
  gl_program.cpp
  gl_texture.cpp
  tessellation_helpers.cpp
)

add_library(mirplatformgraphicscommon OBJECT

  ${GRAPHICS_SOURCES}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android/)
endif()

if (MIR_BUILD_PLATFORM_MESA)
  add_subdirectory(mesa/)
endif()

