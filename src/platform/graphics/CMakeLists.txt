set(DMABUF_PROTO_HEADER ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1_wrapper.h)
set(DMABUF_PROTO_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1_wrapper.cpp)

set(DRM_FORMATS_FILE ${CMAKE_CURRENT_BINARY_DIR}/drm-formats)
set(DRM_FORMATS_BIG_ENDIAN_FILE ${CMAKE_CURRENT_BINARY_DIR}/drm-formats-big-endian)

add_library(mirplatformgraphicscommon OBJECT

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
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/texture.h
  texture.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program.h
  program.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program_factory.h
  program_factory.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_wayland_allocator.h
  egl_wayland_allocator.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/renderer/sw/pixel_source.h
  cpu_buffers.cpp
  egl_logger.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_logger.h
  ${DMABUF_PROTO_HEADER}
  ${DMABUF_PROTO_SOURCE}
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/linux_dmabuf.h
  linux_dmabuf.cpp
  ${DRM_FORMATS_FILE}
  ${DRM_FORMATS_BIG_ENDIAN_FILE}
  drm_formats.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/drm_formats.h
)

set(LINUX_DMABUF_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/protocol/linux-dmabuf-unstable-v1.xml")
set(WAYLAND_GENERATOR "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator")

add_custom_command(
  OUTPUT
  linux-dmabuf-unstable-v1_wrapper.h
  VERBATIM
  COMMAND
  "sh" "-c"
  "${WAYLAND_GENERATOR} zwp_ ${LINUX_DMABUF_PROTO} header > ${DMABUF_PROTO_HEADER}"
  DEPENDS
  ${LINUX_DMABUF_PROTO}
  mir_wayland_generator
)
add_custom_command(
  OUTPUT
  linux-dmabuf-unstable-v1_wrapper.cpp
  VERBATIM
  COMMAND
  "sh" "-c"
  "${WAYLAND_GENERATOR} zwp_ ${LINUX_DMABUF_PROTO} source > ${DMABUF_PROTO_SOURCE}"
  DEPENDS
  ${LINUX_DMABUF_PROTO}
  mir_wayland_generator
)

add_custom_command(
  OUTPUT
    drm-formats
  VERBATIM
  COMMAND
    "sh" "-c"
      "grep \"#define DRM_FORMAT\" /usr/include/libdrm/drm_fourcc.h | \
       grep -v DRM_FORMAT_BIG_ENDIAN |                             \
       grep -v MOD |                                               \
       grep -v DRM_FORMAT_RESERVED |                               \
       tr -s [:blank:] ' ' |                                       \
       cut -f2 -d' ' |                                             \
       sed \"s/\\(.*\\)/STRINGIFY(\\1)/\" >                        \
       ${DRM_FORMATS_FILE}"
)
add_custom_command(
  OUTPUT
    drm-formats-big-endian
  VERBATIM
  COMMAND
    "sh" "-c"
      "grep \"#define DRM_FORMAT\" /usr/include/libdrm/drm_fourcc.h | \
       grep -v DRM_FORMAT_BIG_ENDIAN |                             \
       grep -v MOD |                                               \
       grep -v DRM_FORMAT_RESERVED |                               \
       tr -s [:blank:] ' ' |                                       \
       cut -f2 -d' ' |                                             \
       sed \"s/\\(.*\\)/STRINGIFY_BIG_ENDIAN(\\1)/\" >             \
       ${DRM_FORMATS_BIG_ENDIAN_FILE}"
)

# Here for posterity; the DRM modifier format #defines are a little too
# weird to easily parse via grep ;)
add_custom_command(
  OUTPUT
    drm-modifiers
  VERBATIM
  COMMAND
    "sh" "-c"
      "grep \"#define [[:alnum:]]*_FORMAT_MOD_\\([[:alnum:]]\\|_\\)*[[:space:]]\" /usr/include/drm/drm_fourcc.h | \
       tr -s [:blank:] ' ' |                                              \
       cut -f2 -d' ' |                                                    \
       sed \"s/\\(.*\\)/STRINGIFY(\\1)/\" >                               \
       ${DRM_MODIFIERS_FILE}"
)

target_include_directories(
  mirplatformgraphicscommon

  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/include/platform
  PRIVATE
    ${GL_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

target_link_libraries(
  mirplatformgraphicscommon

  PUBLIC
    mirwayland
    ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  PRIVATE
    mircommon
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
