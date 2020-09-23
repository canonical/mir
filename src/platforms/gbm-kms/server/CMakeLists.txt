add_subdirectory(kms/)

set(DMABUF_PROTO_HEADER ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1_wrapper.h)
set(DMABUF_PROTO_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1_wrapper.cpp)

add_library(
  mirsharedgbmservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  gbm_buffer.cpp
  gbm_platform.cpp
  nested_authentication.cpp
  drm_native_platform.cpp
  ${DMABUF_PROTO_HEADER}
  ${DMABUF_PROTO_SOURCE}
  linux_dmabuf.h
  linux_dmabuf.cpp
)

target_include_directories(
  mirsharedgbmservercommon-static
  PUBLIC
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include/wayland
    ${CMAKE_CURRENT_BINARY_DIR}
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

target_link_libraries(
  mirsharedgbmservercommon-static
  mirsharedgbm-static

  server_platform_common
  kms_utils
  mirwayland
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
)
