set(
  FRONTEND_SOURCES

  authorizing_input_config_changer.cpp
  authorizing_input_config_changer.h
  connection_context.cpp
  no_prompt_shell.cpp
  session_mediator.cpp
  shell_wrapper.cpp
  protobuf_message_processor.cpp
  protobuf_responder.cpp
  protobuf_buffer_packer.cpp
  protobuf_input_converter.cpp
  protobuf_input_converter.h
  published_socket_connector.cpp
  protobuf_connection_creator.cpp
  socket_connection.cpp
  resource_cache.cpp
  socket_messenger.cpp
  event_sender.cpp
  authorizing_display_changer.cpp
  unauthorized_screencast.cpp
  session_credentials.cpp
  default_configuration.cpp
  default_ipc_factory.cpp
  protobuf_ipc_factory.h
  display_server.h
  message_receiver.h
  message_sender.h
  reordering_message_sender.cpp
  reordering_message_sender.h
  event_sink_factory.h
  screencast_buffer_tracker.cpp
  session_mediator_observer_multiplexer.cpp
  session_mediator_observer_multiplexer.h
  basic_mir_client_session.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/shell.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/buffer_stream.h
)

add_library(
  mirfrontend OBJECT

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

