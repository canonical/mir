if (MIR_BUILD_PLATFORM_GBM_KMS OR MIR_BUILD_PLATFORM_GBM_X11 OR MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(kms-utils/)
endif()

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="server_platform_common")

add_library(server_platform_common STATIC
  shm_buffer.cpp
  one_shot_device_observer.h
  one_shot_device_observer.cpp
  cpu_copy_output_surface.cpp
  cpu_copy_output_surface.h
  kms_cpu_addressable_display_provider.cpp
  kms_cpu_addressable_display_provider.h
  cpu_addressable_fb.cpp
  cpu_addressable_fb.h
  kms_framebuffer.h
  options_parsing_helpers.h
  options_parsing_helpers.cpp
  quirk_common.h
  quirk_common.cpp
)

target_include_directories(
  server_platform_common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server_platform_common
  PUBLIC
    mirplatform
    mircommon
    mircore
    ${KMS_UTILS_STATIC_LIBRARY}
    Boost::system
    PkgConfig::WAYLAND_SERVER
)
