include_directories(
    ${LIBHARDWARE_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

add_library(
  mirplatformgraphics SHARED

  android_platform.cpp
  android_buffer_allocator.cpp
  buffer.cpp
  android_display.cpp
  android_display_configuration.cpp
  display_buffer_factory.cpp
  hwc_layerlist.cpp
  hwc10_device.cpp
  hwc11_device.cpp
  hwc_common_device.cpp
  hwc_vsync.cpp
  android_framebuffer_window.cpp
  android_display_factory.cpp
  android_alloc_adaptor.cpp
  server_render_window.cpp
  resource_factory.cpp
  fb_simple_swapper.cpp
  fb_device.cpp
  internal_client_window.cpp
  interpreter_cache.cpp
  internal_client.cpp
)

target_link_libraries(
  mirplatformgraphics

  mirplatform
  mirsharedandroid

  ${LIBHARDWARE_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

install(TARGETS mirplatformgraphics LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
