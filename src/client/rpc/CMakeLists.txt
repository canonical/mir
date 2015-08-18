add_library(mirclientrpc-static STATIC

  mir_basic_rpc_channel.cpp
  null_rpc_report.cpp
  mir_protobuf_rpc_channel.cpp
  make_socket_rpc_channel.cpp
  stream_socket_transport.cpp
  mir_display_server.cpp
  mir_display_server_debug.cpp
)
