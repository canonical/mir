include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${Boost_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
  ${UMOCKDEV_INCLUDE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
)

add_definitions(
    -DMIR_CLIENT_PLATFORM_PATH="${MIR_CLIENT_PLATFORM_PATH}"
    -DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}"
    )

set(
  TEST_FRAMEWORK_SRCS

  async_server_runner.cpp
  executable_path.cpp
  command_line_server_configuration.cpp
  connected_client_headless_server.cpp
  connected_client_with_a_surface.cpp
  cross_process_sync.cpp
  headless_in_process_server.cpp
  headless_test.cpp
  interprocess_client_server_test.cpp
  server_runner.cpp
  temporary_environment_value.cpp
  testing_server_options.cpp
  input_testing_server_options.cpp
  stubbed_server_configuration.cpp
  testing_process_manager.cpp
  testing_client_options.cpp
  display_server_test_fixture.cpp
  process.cpp
  using_stub_client_platform.cpp
  stub_client_platform_factory.cpp
  stub_server_platform_factory.cpp
  udev_environment.cpp
  declarative_placement_strategy.cpp
  fake_event_hub_server_configuration.cpp
  any_surface.cpp
)

list(APPEND TEST_FRAMEWORK_SRCS
  socket_detect_server.cpp
)

# Umockdev uses glib, which uses the deprecated "register" storage qualifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

add_library(
  mir-test-framework STATIC
  ${TEST_FRAMEWORK_SRCS}
)

add_dependencies(mir-test-framework GMock)

uses_android_input(mir-test-framework)

target_link_libraries(
  mir-test-framework

  3rd_party
  mir-test-doubles

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${UMOCKDEV_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-client.map)

add_library(
  mirclientplatformstub MODULE

  stub_client_platform_module.cpp
)

target_link_libraries(
  mirclientplatformstub

  mir-test-framework
  ${UMOCKDEV_LDFLAGS} ${UMOCKDEV_LIBRARIES}
)

set_target_properties(
  mirclientplatformstub PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/client-modules
  OUTPUT_NAME dummy
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

set(server_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-server.map)
add_library(
  mirplatformgraphicsstub MODULE

  platform_graphics_dummy.cpp
  stubbed_graphics_platform.cpp
)

target_link_libraries(
  mirplatformgraphicsstub

  mir-test
  mir-test-framework
  mir-test-doubles
  ${UMOCKDEV_LDFLAGS} ${UMOCKDEV_LIBRARIES}
)

set_target_properties(
  mirplatformgraphicsstub PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/server-modules
  OUTPUT_NAME graphics-dummy
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${server_symbol_map}"
)

add_custom_command(TARGET mir-test-framework POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/udev_recordings ${CMAKE_BINARY_DIR}/bin/udev_recordings
  COMMENT "Copying umockdev recordings to build dir..."
)
add_custom_command(TARGET mir-test-framework POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/testing-cursor-theme ${CMAKE_BINARY_DIR}/bin/testing-cursor-theme
  COMMENT "Copying testing cursor themes to build dir..."
)

string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

install(TARGETS mirplatformgraphicsstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

install(TARGETS mirclientplatformstub LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
