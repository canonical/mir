# TODO(tvoss): These tests are failing for me when run in an emulator
if( MIR_PLATFORM STREQUAL "android")
list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_render.cpp
)
endif()

list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_screencast.cpp
)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
