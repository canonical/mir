if (MIR_BUILD_PLATFORM_GBM_KMS)
  add_subdirectory(kms/)
endif()

include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
)

add_library(
  mirsharedgbmservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  gbm_buffer.cpp
  gbm_platform.cpp
  nested_authentication.cpp
  drm_native_platform.cpp
)

target_link_libraries(
  mirsharedgbmservercommon-static
  mirsharedgbm-static

  server_platform_common
  kms_utils
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
)
