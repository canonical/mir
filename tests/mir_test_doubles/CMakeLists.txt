include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  test_protobuf_client.cpp
  event_factory.cpp
  fake_event_hub.cpp
  fake_event_hub_input_configuration.cpp
  test_protobuf_socket_server.cpp
)

if (MIR_PLATFORM STREQUAL "gbm")
include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
list(APPEND TEST_UTILS_SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
)
endif()

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
