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
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/linux_dmabuf.h
  linux_dmabuf.cpp
  ${DRM_FORMATS_FILE}
  ${DRM_FORMATS_BIG_ENDIAN_FILE}
  drm_formats.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/drm_formats.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_context_executor.h
  egl_context_executor.cpp
  egl_buffer_copy.h
  egl_buffer_copy.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/pixman_image_scaling.h
  pixman_image_scaling.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/drm_syncobj.h
  drm_syncobj.cpp
)

mir_generate_protocol_wrapper(mirplatformgraphicscommon "zwp_" linux-dmabuf-stable-v1.xml)

if (DRM_VERSION VERSION_GREATER 2.4.107)
  target_compile_definitions(
    mirplatformgraphicscommon
    PRIVATE
      MIR_HAVE_DRM_GET_MODIFIER_NAME)
  target_link_libraries(
    mirplatformgraphicscommon
    PUBLIC
      PkgConfig::DRM)
endif()

find_path(DRM_FOURCC_INCLUDE_DIR NAMES "drm_fourcc.h" PATH_SUFFIXES "libdrm" "" HINTS ${DRM_INCLUDE_DIRS} REQUIRED NO_DEFAULT_PATHS)

add_custom_command(
  OUTPUT
    drm-formats
  VERBATIM
  COMMAND
    "sh" "-c"
      "grep \"#define DRM_FORMAT\" ${DRM_FOURCC_INCLUDE_DIR}/drm_fourcc.h | \
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
      "grep \"#define DRM_FORMAT\" ${DRM_FOURCC_INCLUDE_DIR}/drm_fourcc.h | \
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
      "grep \"#define [[:alnum:]]*_FORMAT_MOD_\\([[:alnum:]]\\|_\\)*[[:space:]]\" ${DRM_FOURCC_INCLUDE_DIR}/drm_fourcc.h | \
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
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

target_link_libraries(
  mirplatformgraphicscommon

  PUBLIC
    mirwayland
    PkgConfig::EGL
    PkgConfig::LIBDISPLAY_INFO
  PRIVATE
    mircommon
    PkgConfig::PIXMAN
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
