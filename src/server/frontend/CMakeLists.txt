set(
  FRONTEND_SOURCES

  client_buffer_tracker.cpp
  session_mediator.cpp
  null_session_mediator_report.cpp
  null_message_processor_report.cpp
  protobuf_message_processor.cpp
  protobuf_buffer_packer.cpp
  null_message_processor.cpp
  global_event_sender.cpp

  resource_cache.cpp
  socket_messenger.cpp
  event_sender.cpp
  ${PROTO_HDRS}
)

list(APPEND FRONTEND_SOURCES
  protobuf_socket_communicator.cpp
  make_protobuf_socket_communicator.cpp
  socket_session.cpp
)

if (MIR_PLATFORM STREQUAL "android")
  list(APPEND FRONTEND_SOURCES session_mediator_android.cpp)
elseif (MIR_PLATFORM STREQUAL "gbm")
  include_directories(${DRM_INCLUDE_DIRS})
  list(APPEND FRONTEND_SOURCES session_mediator_gbm.cpp)
endif()

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
