include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(
  mirclientrpc STATIC

  mir_basic_rpc_channel.cpp
  null_rpc_report.cpp
  mir_socket_rpc_channel.cpp
  make_socket_rpc_channel.cpp
  asio_socket_transport.cpp
  ${PROTO_SRCS}
)

target_link_libraries(
  mirclientrpc

  mirprotobuf
  ${PROTOBUF_LIBRARIES}
)
