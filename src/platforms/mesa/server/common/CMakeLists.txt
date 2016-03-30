include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedmesaservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  drm_close_threadsafe.cpp
  drm_mode_resources.cpp
  gbm_buffer.cpp
  ipc_operations.cpp
  $<TARGET_OBJECTS:server_platform_common>
)
