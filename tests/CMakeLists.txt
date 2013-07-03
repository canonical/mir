find_package(umockdev REQUIRED)


if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
  # Avoid clang complaints about poor quality gmock/gtest/cucumber headers
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=null-dereference")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=overloaded-virtual")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/client)
include_directories(${PROJECT_SOURCE_DIR}/include/test)
include_directories(${umockdev_INCLUDE_DIR})

if (MIR_PLATFORM STREQUAL "android")
  include_directories( ${LIBHARDWARE_INCLUDE_DIRS})
endif()

add_subdirectory(acceptance-tests/)
add_subdirectory(unit-tests/)
add_subdirectory(integration-tests/)
add_subdirectory(mir_test_framework/)
add_subdirectory(mir_test_doubles/)
add_subdirectory(draw/)
add_subdirectory(client-language/)
add_subdirectory(behavior-tests/)
add_subdirectory(mir-stress/)

mir_add_memcheck_test()
