include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${UMOCKDEV_INCLUDE_DIRS}
)

set(
  TEST_FRAMEWORK_SRCS

  cross_process_sync.cpp
  in_process_server.cpp
  testing_server_options.cpp
  input_testing_server_options.cpp
  input_testing_client_configuration.cpp
  stubbed_server_configuration.cpp
  testing_process_manager.cpp
  testing_client_options.cpp
  display_server_test_fixture.cpp
  process.cpp
  using_stub_client_platform.cpp
  udev_environment.cpp
  declarative_placement_strategy.cpp
  watchdog.cpp
)

list(APPEND TEST_FRAMEWORK_SRCS
  socket_detect_server.cpp
)

# Umockdev uses glib, which uses the deprecated "register" storage qualifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

add_library(
  mir-test-framework STATIC
  ${TEST_FRAMEWORK_SRCS})
uses_android_input(mir-test-framework)

target_link_libraries(
  mir-test-framework

  mirserver
  mirclient

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${UMOCKDEV_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_custom_command(TARGET mir-test-framework POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/udev_recordings ${CMAKE_BINARY_DIR}/bin/udev_recordings
  COMMENT "Copying umockdev recordings to build dir..."
)
