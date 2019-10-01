include(CMakeDependentOption)

add_definitions(
  -DMIR_CLIENT_PLATFORM_VERSION="${MIR_CLIENT_PLATFORM_VERSION}"
  -DMIR_SERVER_GRAPHICS_PLATFORM_ABI_STRING="${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_KMS)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_X11)
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_definitions(-DMIR_BUILD_PLATFORM_EGLSTREAM_KMS)
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}

  ${DRM_INCLUDE_DIRS}
  ${GBM_INCLUDE_DIRS}
  ${UMOCKDEV_INCLUDE_DIRS}

  ${PROJECT_SOURCE_DIR}/include/cookie
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/src/platforms/common/client
  ${PROJECT_SOURCE_DIR}/src/platforms/common/server
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
)

add_library(example SHARED library_example.cpp)
target_link_libraries(example mircommon)
set_target_properties(
  example PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/shared-libraries
  OUTPUT_NAME example
  PREFIX ""
)

# Umockdev uses glib, which uses the deprecated "register" allocation specifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

set(UMOCK_UNIT_TEST_SOURCES test_udev_wrapper.cpp)

set(
  UNIT_TEST_SOURCES

  test_gmock_fixes.cpp
  test_recursive_read_write_mutex.cpp
  test_glib_main_loop.cpp
  shared_library_test.cpp
  test_raii.cpp
  test_variable_length_array.cpp
  test_default_emergency_cleanup.cpp
  test_thread_safe_list.cpp
  test_fatal.cpp
  test_fd.cpp
  test_flags.cpp
  test_shared_library_prober.cpp
  test_lockable_callback.cpp
  test_module_deleter.cpp
  test_mir_cookie.cpp
  test_posix_rw_mutex.cpp
  test_posix_timestamp.cpp
  test_observer_multiplexer.cpp
  test_edid.cpp
)

if (HAVE_PTHREAD_GETNAME_NP)
  list(APPEND UNIT_TEST_SOURCES test_thread_name.cpp)
else()
  message(WARNING "pthread_getname_np() not supported: Omitting test_thread_name.cpp which relies on it")
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_UNIT_TESTS
  "Run unit tests as part of default testing"
  ON
  "MIR_BUILD_UNIT_TESTS"
  OFF)

add_subdirectory(platforms/)
add_subdirectory(options/)
add_subdirectory(compositor/)
add_subdirectory(console/)
add_subdirectory(logging/)
add_subdirectory(shell/)
add_subdirectory(geometry/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(scene/)
add_subdirectory(thread/)
add_subdirectory(dispatch/)
add_subdirectory(renderers/gl)
add_subdirectory(wayland/)

if (NOT HAVE_PTHREAD_GETNAME_NP)
  set_source_files_properties (
    ${CMAKE_CURRENT_SOURCE_DIR}/dispatch/test_threaded_dispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/test_basic_connector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compositor/test_multi_threaded_compositor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/test_threaded_snapshot_strategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/test_basic_thread_pool.cpp

    PROPERTIES COMPILE_DEFINITIONS MIR_DONT_USE_PTHREAD_GETNAME_NP
  )
  message(WARNING "pthread_getname_np() not supported: Disabling test_threaded_dispatcher.cpp tests that rely on it")
  message(WARNING "pthread_getname_np() not supported: Disabling test_basic_connector.cpp tests that rely on it")
  message(WARNING "pthread_getname_np() not supported: Disabling test_multi_threaded_compositor.cpp tests that rely on it")
  message(WARNING "pthread_getname_np() not supported: Disabling test_threaded_snapshot_strategy.cpp tests that rely on it")
  message(WARNING "pthread_getname_np() not supported: Disabling test_basic_thread_pool.cpp tests that rely on it")
endif()

if(MIR_LIBDRM_HAS_IS_MASTER)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/console/test_minimal_console_services.cpp
    PROPERTIES COMPILE_DEFINITIONS MIR_LIBDRM_HAS_IS_MASTER)
endif()

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

mir_add_wrapped_executable(mir_unit_tests NOINSTALL
  ${UNIT_TEST_SOURCES}
  $<TARGET_OBJECTS:mir-libinput-test-framework>
  $<TARGET_OBJECTS:mir-test-doubles-udev>

  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
)

set_property(
  SOURCE test_udev_wrapper.cpp test_glib_main_loop.cpp
  SOURCE console/test_logind_console_services.cpp
  SOURCE input/test_logind_console_services.cpp input/test_input_platform_probing.cpp
  SOURCE input/evdev/test_evdev_input_platform.cpp
  SOURCE graphics/test_platform_prober.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

mir_precompiled_header(mir_unit_tests ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp)

add_dependencies(mir_unit_tests GMock)

target_link_libraries(
  mir_unit_tests

  exampleserverconfig
  mirdraw
  mircommon
  client_platform_common
  server_platform_common

  mir-test-static
  mir-test-framework-static

  mircommon

  ${PROTOBUF_LITE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${Boost_LIBRARIES}
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11 OR MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  target_link_libraries(
    mir_unit_tests

    mirsharedmesa-static
  )
endif (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11 OR MIR_BUILD_PLATFORM_EGLSTREAM_KMS)

mir_add_wrapped_executable(mir_umock_unit_tests NOINSTALL
  ${UMOCK_UNIT_TEST_SOURCES}
  $<TARGET_OBJECTS:mir-umock-test-framework>
  $<TARGET_OBJECTS:mir-test-input-framework>
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
)

add_dependencies(mir_umock_unit_tests GMock)

target_link_libraries(
  mir_umock_unit_tests

  exampleserverconfig
  mirdraw
  mircommon
  client_platform_common

  mirclient-static
  mirclientlttng-static

  mir-test-static
  mir-test-framework-static

  ${PROTOBUF_LITE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${Boost_LIBRARIES}
  ${UMOCKDEV_LIBRARIES}
  ${LIBINPUT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

target_link_libraries(mir_unit_tests

  mir-test-doubles-static
  mir-test-doubles-platform-static
  )

target_link_libraries(mir_umock_unit_tests

  mir-test-doubles-static
  mir-test-doubles-platform-static
  )

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests G_SLICE=always-malloc G_DEBUG=gc-friendly)
  mir_discover_tests_with_fd_leak_detection(mir_umock_unit_tests LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)

add_custom_command(TARGET mir_unit_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/shared-libraries ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/shared-libraries
  COMMENT "Copying test data to build dir..."
)
add_library(loadable_test_dso SHARED ${CMAKE_CURRENT_SOURCE_DIR}/shared-libraries/empty_input.c)

set_target_properties(loadable_test_dso PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/shared-libraries
  COMPILE_FLAGS "-Wno-pedantic"
  OUTPUT_NAME this-arch
)

add_dependencies(mir_unit_tests loadable_test_dso)

mir_add_test(NAME unit-tests-do-not-link-with-GL-or-EGL
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
      COMMAND /bin/sh -c "!(objdump -p *unit_tests* | grep NEEDED | grep GL)")

add_custom_command(TARGET mir_unit_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/dbus ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/dbus
  COMMENT "Copying dbus-daemon configuration to build dir..."
)
