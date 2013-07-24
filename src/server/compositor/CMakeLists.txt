set(
  MIR_COMPOSITOR_SRCS

  basic_compositing_strategy.cpp
  default_compositing_strategy.cpp
  temporary_buffers.cpp
  buffer_stream_factory.cpp
  buffer_stream_surfaces.cpp
  rendering_operator.cpp
  gl_renderer.cpp
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

