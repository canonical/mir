set(
  FRONTEND_SOURCES

  client_buffer_tracker.cpp
  session_mediator.cpp
  protobuf_message_processor.cpp
  protobuf_responder.cpp
  protobuf_buffer_packer.cpp
  published_socket_connector.cpp
  protobuf_session_creator.cpp
  socket_session.cpp
  resource_cache.cpp
  socket_messenger.cpp
  event_sender.cpp
  surface.cpp
  unauthorized_display_changer.cpp

  default_configuration.cpp

  ${PROTO_HDRS}
)

add_library(
  mirfrontend STATIC

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

target_link_libraries(
  mirfrontend

  mirshell
  mirlogging
  3rd_party
  ${Boost_LIBRARIES}
)
