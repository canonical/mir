include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR})

set(
  MIR_PROTOBUF_PROTOCOL_SPEC
  mir_protobuf_wire.proto
  mir_protobuf.proto
)

protobuf_generate_cpp(
  GENERATED_PROTOBUF_SRCS GENERATED_PROTOBUF_HDRS
  ${MIR_PROTOBUF_PROTOCOL_SPEC}
)

add_library(mirprotobuf SHARED
  google_protobuf_guard.cpp
  ${GENERATED_PROTOBUF_SRCS}
  ${GENERATED_PROTOBUF_HDRS}
)

target_link_libraries(
  mirprotobuf
  ${PROTOBUF_LITE_LIBRARIES}
)

set(MIRPROTOBUF_ABI 2)

set(symbol_map ${CMAKE_SOURCE_DIR}/src/protobuf/symbols.map)

set_target_properties(
  mirprotobuf

  PROPERTIES
  SOVERSION ${MIRPROTOBUF_ABI}
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

# Export the include directories
list(APPEND MIR_GENERATED_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

install(
  TARGETS mirprotobuf
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
