include_directories(
  ${PROJECT_SOURCE_DIR}/include/renderers/gl/
  # TODO: This is a temporary dependency until renderers become proper plugins
  ${PROJECT_SOURCE_DIR}/src/renderers/ 
)

set(
  MIR_COMPOSITOR_SRCS

  default_display_buffer_compositor.cpp
  default_display_buffer_compositor_factory.cpp
  buffer_stream_factory.cpp
  multi_threaded_compositor.cpp
  occlusion.cpp
  default_configuration.cpp
  stream.cpp
  multi_monitor_arbiter.cpp
  dropping_schedule.cpp
  queueing_schedule.cpp
#  basic_screen_shooter.cpp
  null_screen_shooter.cpp
)

ADD_LIBRARY(
  mircompositor OBJECT

  ${MIR_COMPOSITOR_SRCS}
)

target_link_libraries(mircompositor
  PUBLIC
    mirplatform
    mircommon
    mircore
)

