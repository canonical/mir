ADD_LIBRARY(
  mirscene OBJECT

  application_session.cpp
  basic_surface.cpp
  broadcasting_session_event_sink.cpp
  default_configuration.cpp
  session_container.cpp
  mediating_display_changer.cpp
  session_manager.cpp
  surface_allocator.cpp
  surface_stack.cpp
  null_surface_observer.cpp
  null_observer.cpp
  scene_change_notification.cpp
  surface_change_notification.cpp
  rendering_tracker.cpp
  output_properties_cache.cpp
  basic_clipboard.cpp
  surface_state_tracker.cpp
  basic_text_input_hub.cpp
  basic_idle_hub.cpp
  single_surface_scene.cpp
  ${CMAKE_SOURCE_DIR}/src/include/server/mir/scene/surface_observer.h
)

target_include_directories(mirscene
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirscene
  PUBLIC
    mirplatform
    mircommon
    mircore
)
