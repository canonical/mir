if (DEFINED ENV{GTEST_VERSION})
  set(GTEST_VERSION_STR $ENV{GTEST_VERSION})
else()
  unset(GTEST_CONFIG_EXECUTABLE CACHE)
  find_program(GTEST_CONFIG_EXECUTABLE
    NAMES "gtest-config" "gtest-conf")
  if(GTEST_CONFIG_EXECUTABLE)
    message("GTest config found: ${GTEST_CONFIG_EXECUTABLE}")
    EXEC_PROGRAM(${GTEST_CONFIG_EXECUTABLE} "/"
      ARGS "--version"
      OUTPUT_VARIABLE GTEST_VERSION_STR)
  else()
    message(WARNING "Could not detect GTest version (gtest-config not found)")
  endif()
endif()

if (GTEST_VERSION_STR)
  string(REGEX MATCH
    "([0-9])\\.([0-9])\\.([0-9])" _dummy
    ${GTEST_VERSION_STR})
  set(GTEST_VERSION_MAJOR ${CMAKE_MATCH_1})
  set(GTEST_VERSION_MINOR ${CMAKE_MATCH_2})
  set(GTEST_VERSION_PATCH ${CMAKE_MATCH_3})
  message("Detected GTest v${GTEST_VERSION_MAJOR}.${GTEST_VERSION_MINOR}.${GTEST_VERSION_PATCH}")
else()
  message(WARNING "Assuming GTest v1.8.0 (or compatible) and hoping for the best")
  set(GTEST_VERSION_MAJOR 1)
  set(GTEST_VERSION_MINOR 8)
  set(GTEST_VERSION_PATCH 0)
  add_definitions(-DGTEST_VERSION_UNKNOWN)
endif()

add_definitions(-DGTEST_VERSION_MAJOR=${GTEST_VERSION_MAJOR})
add_definitions(-DGTEST_VERSION_MINOR=${GTEST_VERSION_MINOR})
add_definitions(-DGTEST_VERSION_PATCH=${GTEST_VERSION_PATCH})

pkg_check_modules(UMOCKDEV REQUIRED umockdev-1.0>=0.6)
if (NOT UMOCKDEV_FOUND)
  message(FATAL_ERROR "Umockdev not found, cannot build without disabling tests (via MIR_ENABLE_TESTS).")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wnull-dereference HAS_W_NULL_DEREFERENCE)
check_cxx_compiler_flag(-Woverloaded-virtual HAS_W_OVERLOADED_VIRTUAL)
check_cxx_compiler_flag(-Winconsistent-missing-override HAS_W_INCONSISTENT_MISSING_OVERRIDE)
check_cxx_compiler_flag(-Wgnu-zero-variadic-macro-arguments HAS_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
check_cxx_compiler_flag(-Wno-sign-compare HAS_W_GNU_SIGN_COMPARE)

if (HAS_W_NULL_DEREFERENCE)
  # Avoid clang complaints about poor quality gmock/gtest headers
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=null-dereference")
endif()
if (HAS_W_OVERLOADED_VIRTUAL)
  # Avoid clang complaints about poor quality gmock/gtest headers
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=overloaded-virtual")
endif()
if (HAS_W_INCONSISTENT_MISSING_OVERRIDE)
  # MOCK_METHOD()s cannot be marked as override; we cannot consistently mark overrides
  # in the tests.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
endif()
if (HAS_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
  # INSTANTIATE_TEST_CASE_P hits this like a slice of lemon wrapped around a large gold brick.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
else()
  # GCC is a bit less flexible than clang and needs a bigger hammer...
  if (NOT MIR_USE_PRECOMPILED_HEADERS)
    # Work around the Google Test headers not being C++11 friendly (LP: #1658604)
    string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  endif()
endif()
if (HAS_W_GNU_SIGN_COMPARE)
  # TODO fix the code
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-lto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-lto")

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_KMS)
endif()

# public headers (only public headers should be accessed by acceptance tests)
include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/include/server
  ${MIRSERVER_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/test
  ${PROJECT_SOURCE_DIR}/include/renderer
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

option(MIR_BUILD_ACCEPTANCE_TESTS "Build acceptance tests" ON)
option(MIR_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(MIR_BUILD_PERFORMANCE_TESTS "Build performance tests" ON)
option(MIR_BUILD_PRIVILEGED_TESTS "Build privileged tests" ON)
option(MIR_BUILD_UNIT_TESTS "Build unit tests" ON)

if (MIR_BUILD_ACCEPTANCE_TESTS)
  add_subdirectory(acceptance-tests/)
  add_subdirectory(umock-acceptance-tests/)
endif (MIR_BUILD_ACCEPTANCE_TESTS)

if (MIR_BUILD_PERFORMANCE_TESTS)
    add_subdirectory(performance-tests/)
endif(MIR_BUILD_PERFORMANCE_TESTS)

if (MIR_BUILD_PRIVILEGED_TESTS)
    add_subdirectory(privileged-tests/)
endif(MIR_BUILD_PRIVILEGED_TESTS)

add_subdirectory(miral)

# Private test headers used by integration and unit tests
include_directories(
  include
)

if (MIR_BUILD_INTEGRATION_TESTS)
  add_subdirectory(integration-tests/)
endif (MIR_BUILD_INTEGRATION_TESTS)

if (MIR_BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests/)
endif (MIR_BUILD_UNIT_TESTS)

add_subdirectory(mir_test/)
add_subdirectory(mir_test_framework/)
add_subdirectory(mir_test_doubles/)
add_subdirectory(client-language/)
add_subdirectory(test-clients/)
add_subdirectory(loader-tests/)

add_library(mir-test-assist STATIC
  $<TARGET_OBJECTS:mir-public-test>
  $<TARGET_OBJECTS:mir-public-test-doubles>
  $<TARGET_OBJECTS:mir-public-test-framework>
)

target_link_libraries(mir-test-assist
PUBLIC
  mirclient
  miral
  mirserver

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
PRIVATE
  ${GMOCK_MAIN_LIBRARY}
  ${GMOCK_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirtest")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirtest.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirtest.pc
)

install(TARGETS mir-test-assist
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/test/mir
  DESTINATION "include/mirtest"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/test/mir_test_framework
  DESTINATION "include/mirtest"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirtest.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

mir_add_memcheck_test()
mir_add_detect_fd_leaks_test()
