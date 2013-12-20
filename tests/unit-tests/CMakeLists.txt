add_definitions(-DTEST_RECORDINGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/input_recordings/")

include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${UMOCKDEV_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})

set(
  UNIT_TEST_SOURCES

  test_gmock_fixes.cpp
  test_asio_main_loop.cpp
  shared_library_test.cpp
  test_raii.cpp
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
add_subdirectory(scene/)
add_subdirectory(draw/)

add_executable(mir_unit_tests ${UNIT_TEST_SOURCES})
uses_android_input(mir_unit_tests)

target_link_libraries(
  mir_unit_tests

  mirclient
  mirserver
  mirplatformgraphics
  mirdraw
  mirtestdraw
  mirlogging

  mir-test
  mir-test-doubles
  mir-test-doubles-platform
  mir-test-framework

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

if (MIR_PLATFORM STREQUAL "mesa")
  target_link_libraries(
    mir_unit_tests

    ${UMOCKDEV_LIBRARIES}
  )
  # Umockdev uses glib, which uses the deprecated "register" allocation specifier
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")
endif()

mir_discover_tests(mir_unit_tests LD_PRELOAD=libumockdev-preload.so.0)

install(
    TARGETS mir_unit_tests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
