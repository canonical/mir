if (MIR_BUILD_PLATFORM_GBM_KMS OR MIR_BUILD_PLATFORM_GBM_X11 OR MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(kms-utils/)
endif()

include_directories(
  ${server_common_include_dirs}
)

add_library(server_platform_common STATIC
  shm_buffer.cpp
  one_shot_device_observer.h
  one_shot_device_observer.cpp
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
    PkgConfig::GLESv2
)
