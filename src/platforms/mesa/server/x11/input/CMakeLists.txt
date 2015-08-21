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
  mirplatforminputmesax11objects OBJECT

  input.cpp
  input_platform.cpp
  input_device.cpp
  dispatchable.cpp
)
