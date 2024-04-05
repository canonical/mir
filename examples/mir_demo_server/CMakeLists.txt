add_library(exampleserverconfig STATIC
  server_example_input_device_config.cpp
  server_example_input_event_filter.cpp
  server_example_log_options.cpp
  server_example_input_filter.cpp
)

target_link_libraries(exampleserverconfig
  mirserver
  mirwayland
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
)

mir_add_wrapped_executable(mir_demo_server
    server_example.cpp
    glog_logger.cpp
    server_example_test_client.cpp
)

## Needed for google-glog v0.7.0
set_source_files_properties(glog_logger.cpp PROPERTIES COMPILE_DEFINITIONS GLOG_USE_GLOG_EXPORT)

target_link_libraries(mir_demo_server
  example-shell-lib
  exampleserverconfig
  PkgConfig::GLog
  Boost::system
)
