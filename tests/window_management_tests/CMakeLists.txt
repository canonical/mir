include_directories(
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)

string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_NO_LTO_FLAGS})
string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NO_NO_LTO_FLAGS})

mir_add_wrapped_executable(mir_window_management_tests NOINSTALL
  test_minimal_window_manager.cpp
  test_minimal_window_manager_v2.cpp)

target_include_directories(mir_window_management_tests
  PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw)

target_link_libraries(mir_window_management_tests
  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mircommon
  miral

  # GBM platform dependencies
  PkgConfig::DRM
  # Shared platform dependencies
  PkgConfig::EGL
  PkgConfig::GLESv2
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES})

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_WINDOW_MANAGEMENT_TESTS
  "Run window management tests as part of default testing"
  ON
  "MIR_BUILD_WINDOW_MANAGEMENT_TESTS"
  OFF)

if (MIR_RUN_WINDOW_MANAGEMENT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_window_management_tests)
endif (MIR_RUN_WINDOW_MANAGEMENT_TESTS)
