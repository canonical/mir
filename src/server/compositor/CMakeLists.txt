set(
  MIR_COMPOSITOR_SRCS

  basic_display_buffer_compositor.cpp
  default_display_buffer_compositor.cpp
  default_display_buffer_compositor_factory.cpp
  temporary_buffers.cpp
  buffer_stream_factory.cpp
  buffer_stream_surfaces.cpp
  rendering_operator.cpp
  gl_renderer.cpp
  gl_renderer_factory.cpp
  multi_threaded_compositor.cpp
  switching_bundle.cpp
)

ADD_LIBRARY(
  mircompositor STATIC

  ${MIR_COMPOSITOR_SRCS}
)

target_link_libraries(mircompositor
  ${Boost_LIBRARIES}
)

