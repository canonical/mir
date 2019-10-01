include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
)

set(
  INTEGRATION_TESTS_SRCS
  test_focus_selection.cpp
  test_touchspot_visualization.cpp
  test_surface_stack_with_compositor.cpp
  test_buffer_scheduling.cpp
  test_client_screencast.cpp
  test_large_messages.cpp
  test_surfaceloop.cpp
        test_test_framework.cpp
  test_error_reporting.cpp
  test_submit_buffer.cpp
  test_display_info.cpp
  test_display_server_main_loop_events.cpp
  test_surface_first_frame_sync.cpp
  test_swapinterval.cpp
  test_server_client_types.cpp
  test_server_shutdown.cpp
  test_session.cpp
  session_management.cpp
)

add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(shell/)
add_subdirectory(process/)
add_subdirectory(input/)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
mir_add_wrapped_executable(mir_integration_tests NOINSTALL
  ${INTEGRATION_TESTS_SRCS}
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
)

mir_precompiled_header(mir_integration_tests ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp)

add_dependencies(mir_integration_tests GMock)

target_link_libraries(
  mir_integration_tests

  mirclient-static
  mirclientlttng-static

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mirdraw

  mircommon

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  # Mesa platform dependencies
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  # Shared platform dependencies
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${MIR_PLATFORM_REFERENCES}
  ${MIR_SERVER_REFERENCES}
)

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms" OR MIR_TEST_PLATFORM STREQUAL "mesa-x11")
target_link_libraries(mir_integration_tests
  mirsharedmesaservercommon-static
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_INTEGRATION_TESTS
  "Run integration tests as part of default testing"
  ON
  "MIR_BUILD_INTEGRATION_TESTS"
  OFF)

if(MIR_TEST_PLATFORM STREQUAL "mesa-kms" OR MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
  )
  add_subdirectory(graphics/mesa)
endif()

if (MIR_RUN_INTEGRATION_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_integration_tests)
endif (MIR_RUN_INTEGRATION_TESTS)
