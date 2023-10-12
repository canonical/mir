mir_add_wrapped_executable(mir_performance_tests
    test_glmark2-es2.cpp
    test_compositor.cpp
    system_performance_test.cpp
)

target_link_libraries(mir_performance_tests
  mir-test-assist
)

add_dependencies(mir_performance_tests GMock)

add_custom_target(mir-smoke-test-runner ALL
    cp ${PROJECT_SOURCE_DIR}/tools/mir-smoke-test-runner.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

find_program(EGLINFO_EXECUTABLE eglinfo)
find_program(GLMARK2_EXECUTABLE glmark2-es2-wayland)

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_SMOKE_TESTS "Run mir-smoke-test-runner as part of testsuite" ON
  "EGLINFO_EXECUTABLE" OFF
)

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_PERFORMANCE_TESTS "Run mir_performance_tests as part of testsuite" OFF
  "GLMARK2_EXECUTABLE" OFF
)

if(MIR_RUN_SMOKE_TESTS)
  mir_add_test(NAME mir-smoke-test-runner
    COMMAND "env" "MIR_SERVER_VIRTUAL_OUTPUT=1280x1024" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner"
  )
endif()

if(MIR_RUN_PERFORMANCE_TESTS)
  mir_add_test(NAME mir_performance_tests
    COMMAND "env" "MIR_SERVER_VIRTUAL_OUTPUT=1280x1024" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests"
  )
endif()
