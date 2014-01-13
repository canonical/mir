include(CMakeDependentOption)

include_directories(${CMAKE_SOURCE_DIR})

set(
  INTEGRATION_TESTS_SRCS
  test_surfaceloop.cpp
  test_error_reporting.cpp
  test_display_info.cpp
  test_display_server_main_loop_events.cpp
  test_surface_first_frame_sync.cpp
  test_swapinterval.cpp
  test_server_client_types.cpp
  test_session_manager.cpp
  test_session.cpp
)

add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(frontend/)
add_subdirectory(shell/)
add_subdirectory(process/)
add_subdirectory(input/)

if (MIR_PLATFORM STREQUAL "android")
  add_subdirectory(graphics/android)
endif()

if(MIR_PLATFORM STREQUAL "mesa")
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

add_executable(
  mir_integration_tests
  ${INTEGRATION_TESTS_SRCS})
uses_android_input(mir_integration_tests)

target_link_libraries(
  mir_integration_tests

  mir-test
  mir-test-framework
  mir-test-doubles

  mirserver
  mirplatformgraphics
  mirclient
  mirdraw
  mirtestdraw

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

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
