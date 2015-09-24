list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_buffer_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stress_protobuf_communicator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_published_socket_connector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_surface_apis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_reports_errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_buffer_packer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_resource_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session_mediator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_socket_connection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_sender.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_unauthorized_display_changer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_connector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_message_processor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffering_message_sender.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
