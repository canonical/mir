add_custom_target(mir_performance_tests ALL
  cp --remove-destination ${CMAKE_CURRENT_SOURCE_DIR}/performance_tests.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests
)
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms")
  add_definitions(-DMESA_KMS)
endif()
if (MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  add_definitions(-DMESA_X11)
endif()

mir_add_wrapped_executable(mir_glmark2_performance_test
  test_glmark2-es2-mir.cpp
)

target_link_libraries(mir_glmark2_performance_test
  mir-test-assist
)

add_library(mir_system_performance_test STATIC
  system_performance_test.cpp
)

mir_add_wrapped_executable(mir_compositor_performance_test
  test_compositor.cpp
)
target_link_libraries(mir_compositor_performance_test
  mir_system_performance_test
  ${GTEST_BOTH_LIBRARIES}
)

mir_add_wrapped_executable(mir_client_startup_performance_test
  test_client_startup.cpp
)
target_link_libraries(mir_client_startup_performance_test
  mir-test-assist
)

add_dependencies(mir_glmark2_performance_test GMock)
add_dependencies(mir_compositor_performance_test GMock)
add_dependencies(mir_client_startup_performance_test GMock)
