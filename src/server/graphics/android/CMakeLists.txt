include_directories(
    ${LIBHARDWARE_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

add_library(
  mirplatformgraphics STATIC

  android_platform.cpp
  android_buffer_allocator.cpp
  buffer.cpp
  android_buffer_handle.cpp
  android_display.cpp
  hwc_display.cpp
  hwc_layerlist.cpp
  hwc10_device.cpp
  hwc11_device.cpp
  hwc_common_device.cpp
  android_hwc_factory.cpp
  android_display_allocator.cpp
  android_framebuffer_window.cpp
  android_display_factory.cpp
  android_alloc_adaptor.cpp
  server_render_window.cpp
  default_framebuffer_factory.cpp
  fb_simple_swapper.cpp
  fb_device.cpp
)

target_link_libraries(
  mirplatformgraphics
  mircompositor

  ${LIBHARDWARE_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
