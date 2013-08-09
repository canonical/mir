include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirnestedgraphics STATIC

  nested_display.cpp
  nested_display_configuration.cpp
  nested_gl_context.cpp
  nested_gl_context_store.cpp
  nested_mir_connection_handle.cpp
  nested_platform.cpp
  nested_surface_store.cpp
)

target_link_libraries(
  mirnestedgraphics
)
