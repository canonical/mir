mir_add_wrapped_executable(mir_wayland_rs_tests NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_wayland_executor.cpp
)

add_dependencies(mir_wayland_rs_tests GMock)

target_link_libraries(mir_wayland_rs_tests
  mirwayland_rs

  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_wayland_rs_tests)
endif (MIR_RUN_UNIT_TESTS)
