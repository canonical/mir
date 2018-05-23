if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()
if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
)

add_library(
  mirsharedmesaservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  gbm_buffer.cpp
  ipc_operations.cpp
  software_buffer.cpp
  gbm_platform.cpp
  nested_authentication.cpp
  drm_native_platform.cpp
  one_shot_device_observer.cpp
  one_shot_device_observer.h
)

target_link_libraries(
  mirsharedmesaservercommon-static
  mirsharedmesa-static

  server_platform_common
  kms_utils
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
)
