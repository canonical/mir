include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  test_protobuf_client.cpp
  event_factory.cpp
)

list(APPEND TEST_UTILS_SRCS
  fake_event_hub.cpp
  fake_event_hub_input_configuration.cpp
)

list(APPEND TEST_UTILS_SRCS
  test_protobuf_socket_server.cpp
)

add_library(
  mir-test-doubles STATIC
  ${TEST_UTILS_SRCS})
uses_android_input(mir-test-doubles)

target_link_libraries(
  mir-test-doubles

  mirserver

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
