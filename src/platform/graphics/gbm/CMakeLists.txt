include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

# gbm.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


add_library(
  mirplatformgraphics SHARED

  gbm_platform.cpp
  buffer_allocator.cpp
  gbm_buffer.cpp
  gbm_cursor.cpp
  gbm_display_helpers.cpp
  gbm_display.cpp
  real_kms_display_configuration.cpp
  drm_mode_resources.cpp
  gbm_display_buffer.cpp
  real_kms_output.cpp
  real_kms_output_container.cpp
  kms_page_flipper.cpp
  linux_virtual_terminal.cpp
  internal_native_display.cpp
  internal_native_surface.cpp
  internal_client.cpp
  drm_close_threadsafe.cpp
  udev_wrapper.cpp
  overlapping_output_grouping.cpp
  native_gbm_platform.cpp
  anonymous_shm_file.cpp
  shm_buffer.cpp
)

target_link_libraries(
  mirplatformgraphics
  mirplatform

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${UDEV_LDFLAGS} ${UDEV_LIBRARIES}
)

install(TARGETS mirplatformgraphics LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
