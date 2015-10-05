include_directories(
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

add_library(
  mirnestedgraphics OBJECT

  display.cpp
  nested_display_configuration.cpp
  display_buffer.cpp
  mir_client_host_connection.cpp
  cursor.cpp
)
