install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

mir_add_wrapped_executable(mir_performance_tests
    test_glmark2-es2-mir.cpp
    test_compositor.cpp
    test_client_startup.cpp
    system_performance_test.cpp
    test_latency.cpp
)

target_link_libraries(mir_performance_tests
  mir-test-assist
  mirclient-debug-extension
)

add_dependencies(mir_performance_tests GMock)
