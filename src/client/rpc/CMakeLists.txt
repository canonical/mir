add_library(mirclientrpc STATIC

  mir_basic_rpc_channel.cpp
  null_rpc_report.cpp
  mir_protobuf_rpc_channel.cpp
  make_socket_rpc_channel.cpp
  stream_socket_transport.cpp
)
