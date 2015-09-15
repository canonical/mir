include_directories(
  ${PROJECT_SOURCE_DIR}/src/renderers/gl/include
)

set(
  MIR_COMPOSITOR_SRCS

  default_display_buffer_compositor.cpp
  default_display_buffer_compositor_factory.cpp
  temporary_buffers.cpp
  buffer_stream_factory.cpp
  buffer_stream_surfaces.cpp
  gl_renderer.cpp
  gl_renderer_factory.cpp
  gl_program_family.cpp
  multi_threaded_compositor.cpp
  occlusion.cpp
  default_configuration.cpp
  screencast_display_buffer.cpp
  compositing_screencast.cpp
  timeout_frame_dropping_policy_factory.cpp
  buffer_queue.cpp
  recently_used_cache.cpp
  stream.cpp
  multi_monitor_arbiter.cpp
  buffer_map.cpp
  dropping_schedule.cpp
  queueing_schedule.cpp
)

ADD_LIBRARY(
  mircompositor OBJECT

  ${MIR_COMPOSITOR_SRCS}
)

