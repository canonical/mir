include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/server
)

add_library(acceptance-test-throwback OBJECT

  test_client_cursor_api.cpp
)

uses_android_input(acceptance-test-throwback)
