include_directories(
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
add_library(
  mirnestedgraphics OBJECT

  display.cpp
  input_platform.cpp
  nested_display_configuration.cpp
  display_buffer.cpp
  mir_client_host_connection.cpp
  cursor.cpp
  platform.cpp
  buffer.cpp
  surface_spec.cpp
  ipc_operations.cpp
)
