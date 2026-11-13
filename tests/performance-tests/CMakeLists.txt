mir_add_wrapped_executable(mir_performance_tests
    test_glmark2-es2.cpp
    test_compositor.cpp
    performance_test_utils.cpp
)

target_include_directories(mir_performance_tests PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(mir_performance_tests
  mir-test-assist
  mirlogging
)

add_dependencies(mir_performance_tests GMock mir_demo_client_wayland mir_demo_client_wayland_egl_spinner)

add_custom_target(mir-smoke-test-runner ALL
    cp ${PROJECT_SOURCE_DIR}/tools/mir-smoke-test-runner.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_target(mir-compositor-smoke-test ALL
    cp ${PROJECT_SOURCE_DIR}/tools/mir-compositor-smoke-test.bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-compositor-smoke-test
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-compositor-smoke-test
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
    COMMAND "env" "MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual" "MIR_SERVER_VIRTUAL_OUTPUT=1280x1024" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-smoke-test-runner"
  )
endif()

if(MIR_RUN_PERFORMANCE_TESTS)
  mir_add_test(NAME mir_performance_tests
    COMMAND "env" "MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual" "MIR_SERVER_VIRTUAL_OUTPUT=1280x1024" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests" "--gtest_filter=-CompositorPerformance.regression_test_1563287"
  )

  # The virtual platform has no display sinks, so frame timings need a nested X11 display.
  find_program(XVFB_RUN_EXECUTABLE xvfb-run)
  if(XVFB_RUN_EXECUTABLE AND TARGET mirplatformserverx11)
    add_dependencies(mir_performance_tests mirplatformserverx11)
    foreach(logging IN ITEMS false true)
      mir_add_test(NAME mir_compositor_performance_tests_logging_${logging}
        COMMAND "${XVFB_RUN_EXECUTABLE}" "--auto-servernum" "env"
          "MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:x11"
          "MIR_SERVER_X11_OUTPUT=1280x1024"
          "MIR_SERVER_LOGGING=${logging}"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_performance_tests"
          "--enable-input=off"
          "--gtest_filter=CompositorPerformance.regression_test_1563287"
      )
    endforeach()
  else()
    message(STATUS "Compositor performance tests require xvfb-run and the X11 graphics platform")
  endif()
endif()
