set(
  FRONTEND_SOURCES

  client_buffer_tracker.cpp
  connection_context.cpp
  no_prompt_shell.cpp
  session_mediator.cpp
  shell_wrapper.cpp
  protobuf_message_processor.cpp
  protobuf_responder.cpp
  protobuf_buffer_packer.cpp
  published_socket_connector.cpp
  protobuf_connection_creator.cpp
  socket_connection.cpp
  resource_cache.cpp
  socket_messenger.cpp
  event_sender.cpp
  buffer_stream_tracker.cpp
  unauthorized_display_changer.cpp
  unauthorized_screencast.cpp
  session_credentials.cpp
  default_configuration.cpp
  default_ipc_factory.cpp
  unsupported_coordinate_translator.cpp
)

add_library(
  mirfrontend OBJECT

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

