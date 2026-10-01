add_library(
  mirplatformrenderereglcommonobjects OBJECT

  buffer_allocator.cpp
  buffer_allocator.h
  rendering_platform.cpp
  rendering_platform.h
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

  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
