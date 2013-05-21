set(
  MIR_COMPOSITOR_SRCS

  basic_compositing_strategy.cpp
  default_compositing_strategy.cpp
  buffer_basic.cpp
  temporary_buffers.cpp
  buffer_bundle_manager.cpp
  buffer_bundle_surfaces.cpp
  buffer_swapper_multi.cpp
  swapper_factory.cpp
  rendering_operator.cpp
  multi_threaded_compositor.cpp
)

ADD_LIBRARY(
  mircompositor STATIC

  ${MIR_COMPOSITOR_SRCS}
)

target_link_libraries(mircompositor
  ${Boost_LIBRARIES}
)

