include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

add_library(
  mirplatformgraphicsmesax11objects OBJECT

  graphics.cpp
  platform.cpp
  guest_platform.cpp
  display.cpp
  display_configuration.cpp
  gl_context.cpp
  display_group.cpp
  display_buffer.cpp
)
