include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_FRAMEWORK_SRCS

  testing_server_options.cpp
  testing_process_manager.cpp
  display_server_test_fixture.cpp
  process.cpp
)

list(APPEND TEST_FRAMEWORK_SRCS
  socket_detect_server.cpp
)

add_library(
  mir-test-framework STATIC
  ${TEST_FRAMEWORK_SRCS})

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
