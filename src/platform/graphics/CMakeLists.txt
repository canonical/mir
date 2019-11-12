include_directories(${GL_INCLUDE_DIRS})

set(
  GRAPHICS_SOURCES

  egl_extensions.cpp
  egl_resources.cpp
  egl_error.cpp
  display_configuration.cpp
  gamma_curves.cpp
  buffer_basic.cpp
  pixel_format_utils.cpp
  overlapping_output_grouping.cpp
  atomic_frame.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/display.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/wayland_allocator.h
  wayland_allocator.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/texture.h
  texture.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program.h
  program.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program_factory.h
  program_factory.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_wayland_allocator.h
  egl_wayland_allocator.cpp
)

add_library(mirplatformgraphicscommon OBJECT

  ${GRAPHICS_SOURCES}
)

target_include_directories(
  mirplatformgraphicscommon

  PRIVATE
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
