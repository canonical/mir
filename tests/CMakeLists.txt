pkg_check_modules(UMOCKDEV REQUIRED IMPORTED_TARGET umockdev-1.0>=0.6)
if (NOT UMOCKDEV_FOUND)
  message(FATAL_ERROR "Umockdev not found, cannot build without disabling tests (via MIR_ENABLE_TESTS).")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wnull-dereference HAS_W_NULL_DEREFERENCE)
check_cxx_compiler_flag(-Woverloaded-virtual HAS_W_OVERLOADED_VIRTUAL)
check_cxx_compiler_flag(-Winconsistent-missing-override HAS_W_INCONSISTENT_MISSING_OVERRIDE)
check_cxx_compiler_flag(-Wgnu-zero-variadic-macro-arguments HAS_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
check_cxx_compiler_flag(-Wno-sign-compare HAS_W_GNU_SIGN_COMPARE)
check_cxx_compiler_flag(-Wmaybe-uninitialized HAS_W_MAYBE_UNINITIALIZED)
check_cxx_compiler_flag(-Winfinite-recursion HAS_W_INFINITE_RECURSION)

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
  # INSTANTIATE_TEST_SUITE_P hits this like a slice of lemon wrapped around a large gold brick.
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
if (HAS_W_MAYBE_UNINITIALIZED)
  # Avoid g++ complaints about gmock/gtest headers
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=maybe-uninitialized")
endif()
if (HAS_W_INFINITE_RECURSION)
  # g++-12 doesn't notice that Abort() is going to call a [[noreturn]] function
  # before Invalid<T>() tries to return itself.
  add_compile_options(-Wno-error=infinite-recursion)
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-lto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-lto")

include(CheckSymbolExists)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_LIBRARIES "-lpthread")
string(REPLACE " -Werror " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})    # This flag breaks check_symbol_exists()
check_symbol_exists(pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP)
list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES "-lpthread")

list(APPEND CMAKE_REQUIRED_HEADERS ${GTEST_INCLUDE_DIRECTORIES})

if (MIR_BUILD_PLATFORM_GBM_KMS)
  add_compile_definitions(MIR_BUILD_PLATFORM_GBM_KMS)
endif()

# public headers (only public headers should be accessed by acceptance tests)
include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${MIRSERVER_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include/test
  ${PROJECT_SOURCE_DIR}/include/renderer
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

option(MIR_BUILD_ACCEPTANCE_TESTS "Build acceptance tests" ON)
option(MIR_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(MIR_BUILD_WINDOW_MANAGEMENT_TESTS "Build window management tests" ON)
option(MIR_BUILD_PERFORMANCE_TESTS "Build performance tests" ON)
option(MIR_BUILD_MIRAL_TESTS "Build miral tests" ON)
option(MIR_BUILD_UNIT_TESTS "Build unit tests" ON)
option(MIR_BUILD_PLATFORM_TEST_HARNESS "Build platform test harness" ON)

cmake_dependent_option(MIR_BUILD_INTERPROCESS_TESTS
  "Build interprocess acceptance tests as part of default testing" ON
  "MIR_BUILD_ACCEPTANCE_TESTS" OFF)


if (MIR_BUILD_ACCEPTANCE_TESTS)
  add_subdirectory(acceptance-tests/)
  add_subdirectory(umock-acceptance-tests/)
endif (MIR_BUILD_ACCEPTANCE_TESTS)

if (MIR_BUILD_PERFORMANCE_TESTS)
    add_subdirectory(performance-tests/)
endif(MIR_BUILD_PERFORMANCE_TESTS)

if (MIR_BUILD_MIRAL_TESTS)
  add_subdirectory(miral)
endif(MIR_BUILD_MIRAL_TESTS)

# Private test headers used by integration and unit tests
include_directories(
  include
)

if (MIR_BUILD_INTEGRATION_TESTS)
  add_subdirectory(integration-tests/)
endif (MIR_BUILD_INTEGRATION_TESTS)

if (MIR_BUILD_WINDOW_MANAGEMENT_TESTS)
  add_subdirectory(window_management_tests/)
endif(MIR_BUILD_WINDOW_MANAGEMENT_TESTS)

if (MIR_BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests/)
endif (MIR_BUILD_UNIT_TESTS)

if (MIR_BUILD_PLATFORM_TEST_HARNESS)
  add_subdirectory(platform_test_harness/)
endif (MIR_BUILD_PLATFORM_TEST_HARNESS)

add_subdirectory(mir_test/)
add_subdirectory(mir_test_framework/)
add_subdirectory(mir_test_doubles/)

add_library(mir-test-assist STATIC
  $<TARGET_OBJECTS:mir-public-test>
  $<TARGET_OBJECTS:mir-public-test-doubles>
  $<TARGET_OBJECTS:mir-public-test-framework>
)

target_link_libraries(mir-test-assist
PUBLIC
  miral
  mirserver

  Boost::system
  ${GTEST_BOTH_LIBRARIES}
PRIVATE
  ${GMOCK_MAIN_LIBRARY}
  ${GMOCK_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirtest.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirtest.pc
  @ONLY
)

install(TARGETS mir-test-assist
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/test/mir
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirtest"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/test/mir_test_framework
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirtest"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/test/miral
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirtest"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirtest.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

mir_add_memcheck_test()
mir_add_detect_fd_leaks_test()
