add_library(mirplatformtest OBJECT
  test_display_platform.h
  test_display_platform.cpp
  test_rendering_platform.h
  test_rendering_platform.cpp
)

if (MIR_BUILD_PLATFORM_GBM_KMS)
  target_sources(
    mirplatformtest
    PUBLIC
      gbm-kms_platform.cpp)
endif()
if (MIR_BUILD_PLATFORM_X11)
  target_sources(
    mirplatformtest
    PUBLIC
      x11_platform.cpp)
endif()
if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  target_sources(
    mirplatformtest
    PUBLIC
      eglstream-kms_platform.cpp)
endif()
if (MIR_BUILD_PLATFORM_WAYLAND)
  target_sources(
    mirplatformtest
    PUBLIC
      wayland_platform.cpp)
endif()
if (MIR_BUILD_PLATFORM_RPI_DISPMANX)
  target_sources(
    mirplatformtest
    PUBLIC
      rpi-dispmanx_platform.cpp)
endif()

target_compile_definitions(
  mirplatformtest
  PRIVATE
    MIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

target_link_libraries(mirplatformtest
  PUBLIC
    mirplatform
    mircommon
)
