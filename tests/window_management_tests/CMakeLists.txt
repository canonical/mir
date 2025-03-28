# We can't tell which version of gtest we're building against and INSTANTIATE_TEST_SUITE_P changed in
# a way that relies on a gcc extension to support backward-compatible code, So...
check_cxx_compiler_flag(-Wno-gnu-zero-variadic-macro-arguments MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
check_cxx_compiler_flag(-Wno-pedantic MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
if ("${CMAKE_CXX_COMPILER}" MATCHES ".*clang.*" AND MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments") # clang
elseif(MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic") #gcc
endif()

include_directories(
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)

string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_NO_LTO_FLAGS})
string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NO_NO_LTO_FLAGS})

mir_add_wrapped_executable(mir_window_management_tests NOINSTALL
  test_minimal_window_manager.cpp)

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

  Boost::system
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
