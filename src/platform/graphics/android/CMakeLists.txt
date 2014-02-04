include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
include_directories(
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
  display_buffer.cpp
  output_builder.cpp
  hwc_layerlist.cpp
  hwc_layers.cpp
  hwc_fb_device.cpp
  hwc_device.cpp
  hwc_common_device.cpp
  hwc_vsync.cpp
  android_alloc_adaptor.cpp
  server_render_window.cpp
  resource_factory.cpp
  framebuffers.cpp
  fb_device.cpp
  internal_client_window.cpp
  interpreter_cache.cpp
  internal_client.cpp
  gl_context.cpp
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
