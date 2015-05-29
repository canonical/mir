include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedmesaservercommon-static STATIC

  anonymous_shm_file.cpp
  buffer_allocator.cpp
  display_helpers.cpp
  drm_close_threadsafe.cpp
  drm_mode_resources.cpp
  gbm_buffer.cpp
  ipc_operations.cpp
  shm_buffer.cpp
)
