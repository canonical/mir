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

  mirplatformstub
  3rd_party

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

add_library(
  mirplatformstub SHARED
  stubbed_graphics_platform.cpp
)

target_link_libraries(mirplatformstub 
  mirplatform
  mir-test-doubles
  mir-test
)

set_target_properties(mirplatformstub
  PROPERTIES
  LINK_FLAGS "-Wl,--exclude-libs=ALL"
)

install(TARGETS mirplatformstub LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS mirclientplatformstub LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
