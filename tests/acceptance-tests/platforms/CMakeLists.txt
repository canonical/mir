add_library(mirplatformtest OBJECT
  test_display_platform.h
  test_display_platform.cpp
  test_rendering_platform.h
  test_rendering_platform.cpp
  renderer_generic_egl_platform.cpp
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

target_compile_definitions(
  mirplatformtest
  PRIVATE
    MIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

target_include_directories(
  mirplatformtest
  PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/include
)

target_link_libraries(mirplatformtest
  PUBLIC
    mirplatform
    mircommon
    mir-test-doubles-static
)

target_include_directories(mirplatformtest
  PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/include/
)
