pkg_check_modules(UMOCKDEV REQUIRED umockdev-1.0>=0.6)
if (NOT UMOCKDEV_FOUND)
  message(FATAL_ERROR "Umockdev not found, cannot build without disabling tests (via MIR_ENABLE_TESTS).")
endif()

include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wnull-dereference HAS_W_NULL_DEREFERENCE)
check_cxx_compiler_flag(-Woverloaded-virtual HAS_W_OVERLOADED_VIRTUAL)
check_cxx_compiler_flag(-Winconsistent-missing-override HAS_W_INCONSISTENT_MISSING_OVERRIDE)

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

if (MIR_TEST_PLATFORM STREQUAL "android")
  add_definitions(-DANDROID)
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms")
  add_definitions(-DMESA_KMS)
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  add_definitions(-DMESA_X11)
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
  #avoid complaints about poor quality android headers
  include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
  add_definitions(-DMIR_BUILD_PLATFORM_ANDROID)
endif()

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_KMS)
endif()

# public headers (only public headers should be accessed by acceptance tests)
include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/test
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
add_subdirectory(mir-stress/)
add_subdirectory(loader-tests/)

add_library(mir-test-assist STATIC
  $<TARGET_OBJECTS:mir-public-test>
  $<TARGET_OBJECTS:mir-public-test-doubles>
  $<TARGET_OBJECTS:mir-public-test-doubles-platform>
  $<TARGET_OBJECTS:mir-public-test-framework>
)

target_link_libraries(mir-test-assist
  mirclient
  mirserver

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
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
