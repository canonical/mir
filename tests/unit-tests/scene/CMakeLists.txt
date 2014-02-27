list(
  APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_application_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_broadcasting_session_event_sink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_focus_mechanism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_pixel_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_global_event_sender.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_the_session_container_implementation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_threaded_snapshot_strategy.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_stack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_controller.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
