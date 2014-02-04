include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
  GENERATED_PROTOBUF_SRCS GENERATED_PROTOBUF_HDRS
  test_protobuf.proto
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
  test_client_library_drm.cpp
  test_protobuf.cpp
  test_client_screencast.cpp
  ${GENERATED_PROTOBUF_SRCS}
  ${GENERATED_PROTOBUF_HDRS}
)

list(APPEND SOURCES
  test_client_input.cpp)

list(APPEND SOURCES
  test_server_startup.cpp
)

add_executable(
  mir_acceptance_tests

  ${SOURCES}
)
uses_android_input(mir_acceptance_tests)


target_link_libraries(
  mir_acceptance_tests

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

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_ACCEPTANCE_TESTS
  "Run acceptance tests as part of default testing"
  ON
  "MIR_BUILD_ACCEPTANCE_TESTS"
  OFF)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests(mir_acceptance_tests)
endif (MIR_RUN_ACCEPTANCE_TESTS)

install(
    TARGETS mir_acceptance_tests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
