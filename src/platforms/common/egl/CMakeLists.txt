add_library(
  mirplatformrenderereglcommonobjects OBJECT

  buffer_allocator.cpp
  ${PROJECT_SOURCE_DIR}/src/include/platforms/common/egl/buffer_allocator.h
  rendering_platform.cpp
  ${PROJECT_SOURCE_DIR}/src/include/platforms/common/egl/rendering_platform.h
)

target_link_libraries(
  mirplatformrenderereglcommonobjects

  PUBLIC
    mirplatform
    PkgConfig::EGL
    PkgConfig::GLESv2
    PkgConfig::WAYLAND_SERVER
)

target_include_directories(
  mirplatformrenderereglcommonobjects

  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/platforms
)
