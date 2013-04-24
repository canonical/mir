add_definitions(-DTEST_RECORDINGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/input_recordings/")
include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})

set(
  UNIT_TEST_SOURCES

  test_gmock_fixes.cpp
  test_asio_main_loop.cpp
)

add_subdirectory(options/)
add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(frontend/)
add_subdirectory(logging/)
add_subdirectory(shell/)
add_subdirectory(geometry/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(android_input/)
add_subdirectory(surfaces/)
add_subdirectory(draw/)
add_subdirectory(event/)

add_executable(unit-tests ${UNIT_TEST_SOURCES})
uses_android_input(unit-tests)

target_link_libraries(
  unit-tests

  mirclient
  mirserver
  mirdraw
  mirtestdraw
  mirlogging

  mir-test-doubles

  3rd_party

  mirtest_eglmock

  ${PROTOBUF_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(unit-tests)
