include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/src/miral
  ${CMAKE_SOURCE_DIR}
)

add_compile_definitions(
    MIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}"
    MIR_SERVER_GRAPHICS_PLATFORM_ABI_STRING="${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    MIR_SERVER_INPUT_PLATFORM_ABI_STRING="${MIR_SERVER_INPUT_PLATFORM_ABI}"
    MIR_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    MIR_BUILD_PREFIX="${CMAKE_BINARY_DIR}"
    )

# We don't want LTO objects in a published archive (libmir-test-assist.a)
string (REPLACE " -flto " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string (REPLACE " -flto " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_library(mir-public-test-framework OBJECT
  async_server_runner.cpp
  command_line_server_configuration.cpp
  executable_path.cpp
  headless_in_process_server.cpp
  headless_display_buffer_compositor_factory.cpp
  headless_test.cpp
  main.cpp
  passthrough_tracker.cpp       ${PROJECT_SOURCE_DIR}/include/test/mir_test_framework/passthrough_tracker.h
  process.cpp
  server_runner.cpp
  stub_server_platform_factory.cpp
  stub_session.cpp
  stub_surface.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/stubbed_server_configuration.h
  stubbed_server_configuration.cpp
  testing_server_options.cpp
  temporary_environment_value.cpp
  input_device_faker.cpp ${PROJECT_SOURCE_DIR}/include/test/mir_test_framework/input_device_faker.h
  open_wrapper.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/open_wrapper.h
  test_server.cpp  ${PROJECT_SOURCE_DIR}/include/test/miral/test_server.h
  test_display_server.cpp  ${PROJECT_SOURCE_DIR}/include/test/miral/test_display_server.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/mmap_wrapper.h
  mmap_wrapper.cpp
  window_management_test_harness.cpp ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/window_management_test_harness.h
)

target_link_libraries(mir-public-test-framework
  PUBLIC
    mirplatform
    mircommon
    mircore
    miral-external
  PRIVATE
    PkgConfig::UMOCKDEV
)

set_property(
    SOURCE udev_environment.cpp
    PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

if ("libinput_environment.cpp:restrict" IN_LIST MIR_COMPILER_QUIRKS)
  set_property(
    SOURCE libinput_environment.cpp
    PROPERTY COMPILE_OPTIONS -Wno-error=restrict)
endif()

# Umockdev uses glib, which uses the deprecated "register" storage qualifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

add_library(mir-protected-test-framework OBJECT

  fake_input_server_configuration.cpp
  input_testing_server_options.cpp
)

target_link_libraries(mir-protected-test-framework
  PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(mir-libinput-test-framework OBJECT libinput_environment.cpp)
target_link_libraries(mir-libinput-test-framework
  PUBLIC
    mircommon
    mircore
)

add_library(mir-umock-test-framework OBJECT udev_environment.cpp)

target_link_libraries(mir-umock-test-framework
  PRIVATE PkgConfig::UMOCKDEV
)

add_library(mir-test-framework-static STATIC
  $<TARGET_OBJECTS:mir-public-test-framework>
  $<TARGET_OBJECTS:mir-protected-test-framework>
  $<TARGET_OBJECTS:mir-public-test>
)

add_dependencies(mir-test-framework-static GMock)

target_link_libraries(mir-test-framework-static

  mirserver
  Boost::system
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  PkgConfig::UMOCKDEV
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-client.map)

add_library(
  mir-test-input-framework OBJECT

  stub_input.cpp
  fake_input_device_impl.cpp
  stub_input_platform.cpp
  )

target_link_libraries(mir-test-input-framework
  PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(
  mirplatforminputstub MODULE
  $<TARGET_OBJECTS:mir-test-input-framework>
  $<TARGET_OBJECTS:mirevdevutilsobjects>)
target_link_libraries(mirplatforminputstub mircommon)

set_target_properties(
  mirplatforminputstub PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME input-stub
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${MIR_INPUT_PLATFORM_VERSION_SCRIPT}"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols-server.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols-server.map)
set(server_symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols-server.map)
add_library(
  mirplatformgraphicsstub MODULE

  platform_graphics_dummy.cpp
  stubbed_graphics_platform.cpp
)

target_link_libraries(
  mirplatformgraphicsstub

  PRIVATE
  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  PUBLIC
  PkgConfig::UMOCKDEV
)

set_target_properties(
  mirplatformgraphicsstub PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME graphics-dummy
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${server_symbol_map}"
)

install(TARGETS mirplatformgraphicsstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
install(TARGETS mirplatforminputstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

add_library(
  mirplatformgraphicsthrow MODULE
  platform_graphics_throw.cpp
)

target_link_libraries(
  mirplatformgraphicsthrow

  mirplatform
)

set_target_properties(
  mirplatformgraphicsthrow PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME graphics-throw
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${server_symbol_map}"
)

add_custom_command(TARGET mir-test-framework-static POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/udev-recordings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/udev-recordings
  COMMENT "Copying umockdev recordings to build dir..."
)
add_custom_command(TARGET mir-test-framework-static POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/testing-cursor-theme ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-data/testing-cursor-theme
  COMMENT "Copying testing cursor themes to build dir..."
)

string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

add_subdirectory(udev-recordings/)
