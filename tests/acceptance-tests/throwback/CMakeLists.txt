include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/tests/include
)

add_library(acceptance-test-throwback OBJECT

  test_client_cursor_api.cpp
)

uses_android_input(acceptance-test-throwback)
