set(GENERATOR ${CMAKE_BINARY_DIR}/bin/wrapper-generator)
set(PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/protocol/server-decoration.xml")
set(OUTPUT_PATH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.h")

add_custom_command(
    OUTPUT server-decoration_wrapper.cpp
    OUTPUT server-decoration_wrapper.h
    DEPENDS wrapper-generator
    DEPENDS ${PROTOCOL}
    COMMAND "sh" "-c" "${GENERATOR} org_kde_kwin_ ${PROTOCOL} header >server-decoration_wrapper.h"
    COMMAND "sh" "-c" "${GENERATOR} org_kde_kwin_ ${PROTOCOL} source >server-decoration_wrapper.cpp"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(exampleserverconfig STATIC
  server_example_input_device_config.cpp
  server_example_input_event_filter.cpp
  server_example_log_options.cpp
  server_example_input_filter.cpp
  server_example_host_lifecycle_event.cpp
  server_example_custom_compositor.cpp
  server_example_adorning_compositor.cpp
  server_example_decoration.cpp server_example_decoration.h
  server-decoration_wrapper.cpp server-decoration_wrapper.h
  server_example_decoration.cpp server_example_decoration.h
)

target_link_libraries(exampleserverconfig
  mirserver
  mirwayland
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${GL_INCLUDE_DIRS}
)

mir_add_wrapped_executable(mir_demo_server
    server_example.cpp
    glog_logger.cpp
    server_example_test_client.cpp
)

target_link_libraries(mir_demo_server
  example-shell-lib
  exampleserverconfig
  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
  ${Boost_LIBRARIES}
)
