list(
  APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_application_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_broadcasting_session_event_sink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_the_session_container_implementation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_mediating_display_changer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_session_container.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_session_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_session_impl.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/test_abstract_shell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_stack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_scene_change_notification.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_clipboard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_state_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_text_input_hub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_idle_hub.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
