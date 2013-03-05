include_directories(
  ${CMAKE_SOURCE_DIR}
)

set(
  SOURCES

  test_client_library.cpp
  test_surfaceloop.cpp
  test_test_framework.cpp
  test_focus_management_api.cpp
)

# TODO Disabled in binder version as this test uses detect_server() and that
#      can't be called in the parent process without subsequent client
#      processes segfaulting when they call checkService()/getService().
if (NOT MIR_TRANSPORT STREQUAL "binder")
list(APPEND SOURCES
  test_server_startup.cpp
)
endif()

add_executable(
  acceptance-tests

  ${SOURCES}
)


target_link_libraries(
  acceptance-tests

  mirserver
  mirclient

  mir-test-framework
  mir-test-doubles
  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${ANDROID_STDLIB}
)

mir_discover_tests(acceptance-tests)
