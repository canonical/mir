include_directories(
    ${LIBHARDWARE_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

add_library(
  mirplatformgraphics STATIC

  android_platform.cpp
  android_buffer_allocator.cpp
  android_buffer.cpp
  android_buffer_handle.cpp
  android_display.cpp
  hwc_display.cpp
  android_display_selector.cpp
  android_framebuffer_window.cpp
  android_fb_factory.cpp
  android_alloc_adaptor.cpp
)

target_link_libraries(
  mirplatformgraphics
  mircompositor

  3rd_party_fbtype

  ${LIBHARDWARE_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
