add_definitions(-DUDEVMOCK_DIR="${CMAKE_CURRENT_SOURCE_DIR}/udev_recordings")

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
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
)

list(APPEND TEST_FRAMEWORK_SRCS
  socket_detect_server.cpp
)

if (UMOCKDEV_REQUIRED)
  include_directories(${UMOCKDEV_INCLUDE_DIRS})
  list(APPEND TEST_FRAMEWORK_SRCS udev_environment.cpp)
  list(APPEND TEST_FRAMEWORK_LIBS ${UMOCKDEV_LIBRARIES})
  # Umockdev uses glib, which uses the deprecated "register" storage qualifier
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")
endif ()

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
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

target_link_libraries(mir-test-framework
  ${TEST_FRAMEWORK_LIBS}
)
