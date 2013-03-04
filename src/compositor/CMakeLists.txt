set(
  MIR_COMPOSITOR_SRCS

  compositor.cpp
  buffer_basic.cpp
  temporary_buffers.cpp
  buffer_bundle_manager.cpp
  buffer_bundle_surfaces.cpp
  buffer_swapper_multi.cpp
  swapper_factory.cpp
  rendering_operator.cpp
)

ADD_LIBRARY(
  mircompositor STATIC

  ${MIR_COMPOSITOR_SRCS}
)

target_link_libraries(mircompositor
  ${Boost_LIBRARIES}
)

