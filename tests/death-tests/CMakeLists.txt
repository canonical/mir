add_executable(
  death-tests
  test_swapper_factory.cpp
  test_process_death.cpp
  test_application_manager_death.cpp
)

target_link_libraries(
  death-tests

  mirserver
  mirclient
  mir-test-framework

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(death-tests)
