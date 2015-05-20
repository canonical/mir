#include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})

#add_definitions( -DANDROID )

include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedmesaservercommon-static STATIC

  buffer_allocator.cpp
  drm_mode_resources.cpp
  anonymous_shm_file.cpp
  shm_buffer.cpp
  ipc_operations.cpp
  gbm_buffer.cpp
  display_helpers.cpp
  drm_close_threadsafe.cpp
)
