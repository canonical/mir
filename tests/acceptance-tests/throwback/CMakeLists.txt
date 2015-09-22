include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/tests/include
)

add_library(acceptance-test-throwback OBJECT

  test_client_cursor_api.cpp     # uses BasicWindowManager<>
  test_client_library_errors.cpp # uses UsingClientPlatform
  test_shell_control_of_surface_configuration.cpp # uses CanonicalWindowManager
)
