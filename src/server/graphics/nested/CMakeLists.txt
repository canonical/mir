include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirnestedgraphics STATIC

  nested_display.cpp
  nested_display_configuration.cpp
  nested_platform.cpp
  nested_mir_connection_handle.cpp
)

target_link_libraries(
  mirnestedgraphics
)
