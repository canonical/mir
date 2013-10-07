include_directories(
  ${CMAKE_SOURCE_DIR}
)

set(
  SOURCES

  test_client_library.cpp
  test_surfaceloop.cpp
  test_test_framework.cpp
  test_focus_selection.cpp
  test_server_shutdown.cpp
  test_client_focus_notification.cpp
  test_client_authorization.cpp
  test_shell_control_of_surface_configuration.cpp
  test_nested_mir.cpp
  test_display_configuration.cpp
  test_surfaces_with_output_id.cpp
  test_server_disconnect.cpp
)

list(APPEND SOURCES
  test_client_input.cpp)

list(APPEND SOURCES
  test_server_startup.cpp
)

add_executable(
  acceptance-tests

  ${SOURCES}
)
uses_android_input(acceptance-tests)


target_link_libraries(
  acceptance-tests

  mirserver
  mirclient
  mirplatform

  mir-test
  mir-test-framework
  mir-test-doubles
  mir-test-doubles-platform
  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(acceptance-tests)
