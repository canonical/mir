include_directories(${GLESv2_INCLUDE_DIRS})

set(
  GRAPHICS_SOURCES

  egl_extensions.cpp
  egl_resources.cpp
  egl_error.cpp
  display_configuration.cpp
  buffer_basic.cpp
  pixel_format_utils.cpp
  overlapping_output_grouping.cpp
  gl_program.cpp
  gl_texture.cpp
  tessellation_helpers.cpp
  platform_probe.cpp
)

add_library(mirplatformgraphicscommon OBJECT

  ${GRAPHICS_SOURCES}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
