include(CMakeDependentOption)

add_definitions(
  -DTEST_RECORDINGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/input_recordings/"
  -DMIR_CLIENT_PLATFORM_VERSION="${MIR_CLIENT_PLATFORM_VERSION}"
)

if (MIR_BUILD_PLATFORM_ANDROID)
  add_definitions(-DMIR_BUILD_PLATFORM_ANDROID)
endif()

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_KMS)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_definitions(-DMIR_BUILD_PLATFORM_MESA_X11)
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}

  ${ANDROID_HEADERS_INCLUDE_DIRS}
  ${DRM_INCLUDE_DIRS}
  ${GBM_INCLUDE_DIRS}
  ${UMOCKDEV_INCLUDE_DIRS}

  ${PROJECT_SOURCE_DIR}/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/src/platforms/common/client
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
  ${GLIB_INCLUDE_DIRS}
)

# TODO: Aim to remove
include_directories(${MIR_XCURSOR_INCLUDE_DIRECTORIES})

add_library(example SHARED library_example.cpp)
target_link_libraries(example mircommon)
set_target_properties(
  example PROPERTIES
  OUTPUT_NAME example
  PREFIX ""
)

# Umockdev uses glib, which uses the deprecated "register" allocation specifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

set(
  UNIT_TEST_SOURCES

  test_gmock_fixes.cpp
  test_recursive_read_write_mutex.cpp
  test_glib_main_loop.cpp
  shared_library_test.cpp
  test_raii.cpp
  test_udev_wrapper.cpp
  test_variable_length_array.cpp
  test_thread_name.cpp
  test_default_emergency_cleanup.cpp
  test_thread_safe_list.cpp
  test_fatal.cpp
  test_fd.cpp
  test_flags.cpp
  test_shared_library_prober.cpp
  test_lockable_callback.cpp
  test_module_deleter.cpp
  test_mir_cookie.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa-x11")
list(APPEND UNIT_TEST_SOURCES
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/x11/X11_resources.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
  $<TARGET_OBJECTS:mirplatforminputmesax11objects>
)
endif()

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
add_subdirectory(thread/)
add_subdirectory(dispatch/)
add_subdirectory(renderers/gl)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

mir_add_wrapped_executable(mir_unit_tests
  ${UNIT_TEST_SOURCES}
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
)

mir_precompiled_header(mir_unit_tests ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp)

add_dependencies(mir_unit_tests GMock)

uses_android_input(mir_unit_tests)

target_link_libraries(
  mir_unit_tests

  mirdraw
  demo-shell
  mircommon

  mirclient-static
  mirclientrpc-static
  mirclientlttng-static

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static

  xcursorloader-static

  ${PROTOBUF_LITE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${Boost_LIBRARIES}
  ${UMOCKDEV_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${MIR_PLATFORM_REFERENCES}
  ${MIR_COMMON_REFERENCES}
  ${MIR_SERVER_REFERENCES}
  ${XKBCOMMON_LIBRARIES}
)

if ((MIR_BUILD_PLATFORM_MESA_KMS) OR (MIR_BUILD_PLATFORM_MESA_X11))
target_link_libraries(mir_unit_tests
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  mirsharedmesaservercommon-static)
endif()

if (MIR_BUILD_PLATFORM_MESA_KMS)
target_link_libraries(mir_unit_tests
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
target_link_libraries(mir_unit_tests 
  mirsharedandroid-static
  ${LIBHARDWARE_LDFLAGS} ${LIBHARDWARE_LIBRARIES}
  ${ANDROID_PROPERTIES_LDFLAGS} ${ANDROID_PROPERTIES_LIBRARIES}
)
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_UNIT_TESTS
  "Run unit tests as part of default testing"
  ON
  "MIR_BUILD_UNIT_TESTS"
  OFF)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)

add_custom_command(TARGET mir_unit_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/test_data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data
  COMMENT "Copying test data to build dir..."
)

add_library(loadable_test_dso SHARED ${CMAKE_CURRENT_SOURCE_DIR}/test_data/empty_input.c)

set_target_properties(loadable_test_dso PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data
  COMPILE_FLAGS "-Wno-pedantic"
  OUTPUT_NAME this-arch
)

add_dependencies(mir_unit_tests loadable_test_dso)
