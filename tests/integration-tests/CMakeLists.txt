include(CMakeDependentOption)

include_directories(${CMAKE_SOURCE_DIR})

set(
  INTEGRATION_TESTS_SRCS
  test_surface_stack_with_compositor.cpp
  test_surfaceloop.cpp
  test_error_reporting.cpp
  test_display_info.cpp
  test_display_server_main_loop_events.cpp
  test_surface_first_frame_sync.cpp
  test_swapinterval.cpp
  test_server_client_types.cpp
  test_session_manager.cpp
  test_session.cpp
  session_management.cpp
)

add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(frontend/)
add_subdirectory(shell/)
add_subdirectory(process/)
add_subdirectory(input/)

if (MIR_TEST_PLATFORM STREQUAL "android")
  add_subdirectory(graphics/android)
endif()

if(MIR_TEST_PLATFORM STREQUAL "mesa")
  include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
  )
  add_subdirectory(graphics/mesa)
  list(APPEND INTEGRATION_TESTS_SRCS test_drm_auth_magic.cpp)
endif()

link_directories(${LIBRARY_OUTPUT_PATH})
add_executable(
  mir_integration_tests
  ${INTEGRATION_TESTS_SRCS}
  ${MIR_SERVER_OBJECTS}
  $<TARGET_OBJECTS:mirclientobjects>
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
)

add_dependencies(mir_integration_tests GMock)

uses_android_input(mir_integration_tests)

target_link_libraries(
  mir_integration_tests

  mir-test
  mir-test-framework
  mir-test-doubles

  mirdraw
  mirtestdraw
  mirclientrpc
  mirclientlttngstatic

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${MIR_PLATFORM_REFERENCES}
  ${MIR_COMMON_REFERENCES}
  ${MIR_SERVER_REFERENCES}
)

if (MIR_TEST_PLATFORM STREQUAL "android")
target_link_libraries(mir_integration_tests
  mirsharedandroid
  ${ANDROID_PROPERTIES_LDFLAGS}
)
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_INTEGRATION_TESTS
  "Run integration tests as part of default testing"
  ON
  "MIR_BUILD_INTEGRATION_TESTS"
  OFF)

if (MIR_RUN_INTEGRATION_TESTS)
  mir_discover_tests(mir_integration_tests)
endif (MIR_RUN_INTEGRATION_TESTS)

install(
    TARGETS mir_integration_tests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
