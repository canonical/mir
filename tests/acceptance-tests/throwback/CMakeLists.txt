include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
)

add_library(acceptance-test-throwback OBJECT

  clients.cpp
  test_client_cursor_api.cpp
  test_client_input.cpp
  test_custom_input_dispatcher.cpp
  test_focus_selection.cpp
  test_shell_control_of_surface_configuration.cpp
  test_touchspot_visualization.cpp
)

uses_android_input(acceptance-test-throwback)
