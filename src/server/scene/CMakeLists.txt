ADD_LIBRARY(
  mirscene STATIC

  application_session.cpp
  broadcasting_session_event_sink.cpp
  default_configuration.cpp
  default_session_container.cpp
  gl_pixel_buffer.cpp
  global_event_sender.cpp
  mediating_display_changer.cpp
  session_manager.cpp
  surface.cpp
  surface_allocator.cpp
  surface_data.cpp
  surface_stack.cpp
  surface_controller.cpp
  scene_report.cpp
  surface_source.cpp
  surface_impl.cpp
  threaded_snapshot_strategy.cpp
)
