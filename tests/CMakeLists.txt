if(NOT DEFINED ENV{MIR_TESTS_USE_REAL_GRAPHICS})
  message(STATUS "Tests are NOT run with real graphics support.")
else()
  message(STATUS "Tests are run with real graphics.")
endif()

if(NOT DEFINED ENV{MIR_TESTS_USE_REAL_INPUT})
  message(STATUS "Tests are NOT run with real input.")
else()
  message(STATUS "Tests are run with real input.")
endif()

option(
  MIR_ENABLE_DEATH_TESTS
  "Option to control whether to run death tests"
  ON
)

if(MIR_ENABLE_DEATH_TESTS)
  add_definitions(-DMIR_DEATH_TESTS_ENABLED)
endif(MIR_ENABLE_DEATH_TESTS)

include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/client)
include_directories(${PROJECT_SOURCE_DIR}/include/test)

if (MIR_PLATFORM STREQUAL "android")
  include_directories( ${LIBHARDWARE_INCLUDE_DIRS} ${ANDROID_UI_INCLUDE_DIRS})
endif()

add_subdirectory(acceptance-tests/)
add_subdirectory(unit-tests/)
add_subdirectory(integration-tests/)
add_subdirectory(mir_test_framework/)
add_subdirectory(mir_test_doubles/)
add_subdirectory(draw/)
add_subdirectory(client-language/)
add_subdirectory(behavior-tests/)

if(MIR_ENABLE_DEATH_TESTS)
  add_subdirectory(death-tests/)
else()
  message(STATUS "Death tests are disabled and are not run.")
endif(MIR_ENABLE_DEATH_TESTS)

mir_add_memcheck_test()
