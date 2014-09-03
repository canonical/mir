include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirnestedgraphics OBJECT

  nested_display.cpp
  nested_display_configuration.cpp
  nested_output.cpp
  nested_platform.cpp
  mir_client_host_connection.cpp
)

