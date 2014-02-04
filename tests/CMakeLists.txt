
if (MIR_PLATFORM STREQUAL "mesa")
  set (UMOCKDEV_REQUIRED YES)
endif()

if (UMOCKDEV_REQUIRED)
  pkg_check_modules(UMOCKDEV REQUIRED umockdev-1.0)
  if (NOT UMOCKDEV_FOUND)
    message(FATAL_ERROR "Umockdev not found, cannot build without disabling tests (via MIR_ENABLE_TESTS).")
  endif()
endif()

include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
  # Avoid clang complaints about poor quality gmock/gtest headers
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=null-dereference")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=overloaded-virtual")
endif()

if (MIR_PLATFORM STREQUAL "android")
  #avoid complaints about poor quality android headers
  include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/test)

option(MIR_BUILD_ACCEPTANCE_TESTS "Build acceptance tests" ON)
option(MIR_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(MIR_BUILD_UNIT_TESTS "Build unit tests" ON)

if (MIR_BUILD_ACCEPTANCE_TESTS)
  add_subdirectory(acceptance-tests/)
endif (MIR_BUILD_ACCEPTANCE_TESTS)

if (MIR_BUILD_INTEGRATION_TESTS)
  add_subdirectory(integration-tests/)
endif (MIR_BUILD_INTEGRATION_TESTS)

if (MIR_BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests/)
endif (MIR_BUILD_UNIT_TESTS)

add_subdirectory(mir_test/)
add_subdirectory(mir_test_framework/)
add_subdirectory(mir_test_doubles/)
add_subdirectory(draw/)
add_subdirectory(client-language/)
add_subdirectory(mir-stress/)

mir_add_memcheck_test()
