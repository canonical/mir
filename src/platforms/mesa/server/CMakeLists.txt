if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()
if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

if (GBM_VERSION VERSION_LESS 11)
  message(WARNING "Hybrid support requires libgbm from Mesa 11.0 or greater. Hybrid setups will not work")
  add_definitions(-DMIR_NO_HYBRID_SUPPORT)
endif()

include_directories(
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedmesaservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  drm_close_threadsafe.cpp
  gbm_buffer.cpp
  ipc_operations.cpp
  software_buffer.cpp
)

target_link_libraries(
  mirsharedmesaservercommon-static
  mirsharedmesa-static

  server_platform_common
  kms_utils
)
