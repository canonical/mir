include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client

    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  test_protobuf_client.cpp
  event_factory.cpp
  fake_event_hub.cpp
  test_protobuf_socket_server.cpp
  mock_timer.cpp
  mock_frame_dropping_policy_factory.cpp
  platform_factory.cpp
)

set(
  MIR_TEST_DOUBLES_PLATFORM_SRCS

  ${CMAKE_CURRENT_SOURCE_DIR}/mock_egl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gl.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
)
elseif (MIR_TEST_PLATFORM STREQUAL "android")
list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_android_hw.cpp
)
endif()

add_library(
  mir-test-doubles STATIC
  ${TEST_UTILS_SRCS})

add_dependencies(mir-test-doubles GMock)

uses_android_input(mir-test-doubles)

target_link_libraries(
  mir-test-doubles

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_library(
  mir-test-doubles-platform STATIC
  ${MIR_TEST_DOUBLES_PLATFORM_SRCS})

target_link_libraries(
  mir-test-doubles-platform

  -ldl
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
