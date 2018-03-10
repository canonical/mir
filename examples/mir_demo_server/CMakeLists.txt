add_library(exampleserverconfig STATIC
  server_example_input_device_config.cpp
  server_example_input_event_filter.cpp
  server_example_log_options.cpp
  server_example_input_filter.cpp
  server_example_host_lifecycle_event.cpp
  server_example_custom_compositor.cpp
  server_example_adorning_compositor.cpp
)

target_link_libraries(exampleserverconfig mirserver)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${GL_INCLUDE_DIRS}
)

add_library(mir_demo_server_loadable MODULE
  server_example.cpp
  glog_logger.cpp
  server_example_test_client.cpp
)

target_link_libraries(mir_demo_server_loadable
  miral-shell-lib
  miral
  exampleserverconfig
  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
  ${Boost_LIBRARIES}
)

install(TARGETS mir_demo_server_loadable
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

mir_add_wrapped_executable(mir_demo_server
  mir_demo_server_loader.cpp
)

target_link_libraries(mir_demo_server
  dl
)
