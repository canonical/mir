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
  screencast_display_buffer.cpp
  compositing_screencast.cpp
  stream.cpp
  multi_monitor_arbiter.cpp
  dropping_schedule.cpp
  queueing_schedule.cpp
)

# TODO this is a frig to workaround the lack of a way for the screencast client to ask for software buffers
if (MIR_EGL_SUPPORTED)
    set_source_files_properties(compositing_screencast.cpp PROPERTIES COMPILE_DEFINITIONS MIR_EGL_SUPPORTED)
endif()

ADD_LIBRARY(
  mircompositor OBJECT

  ${MIR_COMPOSITOR_SRCS}
)

