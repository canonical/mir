add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="demo_server")

add_library(exampleserverconfig STATIC
  server_example_input_event_filter.cpp
  server_example_input_filter.cpp
)

target_include_directories(exampleserverconfig
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(exampleserverconfig
  mirserver
)

mir_add_wrapped_executable(mir_demo_server
    server_example.cpp
    server_example_test_client.cpp
)

target_include_directories(mir_demo_server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mir_demo_server
  example-shell-lib
  exampleserverconfig
)
