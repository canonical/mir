add_subdirectory(kms/)

add_library(
  mirsharedgbmservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  gbm_buffer.cpp
  gbm_platform.cpp
  nested_authentication.cpp
  drm_native_platform.cpp
)

target_include_directories(
  mirsharedgbmservercommon-static
  PUBLIC
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
  mirsharedgbmservercommon-static
  mirsharedgbm-static

  server_platform_common
  kms_utils
  mirplatform
  mirwayland
)
