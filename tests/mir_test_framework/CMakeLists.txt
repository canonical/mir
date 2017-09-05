include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${Boost_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
  ${UMOCKDEV_INCLUDE_DIRS}
)

add_definitions(
    -DMIR_CLIENT_PLATFORM_PATH="${MIR_CLIENT_PLATFORM_PATH}"
    -DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}"
    -DMIR_CLIENT_PLATFORM_ABI_STRING="${MIR_CLIENT_PLATFORM_ABI}"
    -DMIR_SERVER_GRAPHICS_PLATFORM_ABI_STRING="${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    -DMIR_SERVER_INPUT_PLATFORM_ABI_STRING="${MIR_SERVER_INPUT_PLATFORM_ABI}"
    -DMIR_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    -DMIR_BUILD_PREFIX="${CMAKE_BINARY_DIR}"
    )

# We don't want LTO objects in a published archive (libmir-test-assist.a)
string (REPLACE " -flto " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string (REPLACE " -flto " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_library(mir-public-test-framework OBJECT
  ${PROJECT_SOURCE_DIR}/include/test/mir_test_framework/any_surface.h
  any_surface.cpp
  async_server_runner.cpp
  command_line_server_configuration.cpp
  connected_client_headless_server.cpp
  connected_client_with_a_window.cpp
  declarative_placement_window_manage_policy.cpp
  executable_path.cpp
  headless_in_process_server.cpp
  headless_nested_server_runner.cpp
  headless_display_buffer_compositor_factory.cpp
  headless_test.cpp
  interprocess_client_server_test.cpp
  main.cpp
  observant_shell.cpp
  placement_applying_shell.cpp
  process.cpp
  server_runner.cpp
  socket_detect_server.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/stub_client_platform_factory.h
  stub_client_platform_factory.cpp
  stub_server_platform_factory.cpp
  stub_session.cpp
  stub_surface.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir_test_framework/stubbed_server_configuration.h
  stubbed_server_configuration.cpp
  testing_client_options.cpp
  testing_server_options.cpp
  temporary_environment_value.cpp
  visible_surface.cpp
  stub_client_platform_options.cpp
  ${PROJECT_SOURCE_DIR}/include/test/mir_test_framework/stub_client_platform_options.h
  basic_window_manager.cpp  ${PROJECT_SOURCE_DIR}/include/test/mir/shell/basic_window_manager.h
  canonical_window_manager.cpp  ${PROJECT_SOURCE_DIR}/include/test/mir/shell/canonical_window_manager.h
)

# Umockdev uses glib, which uses the deprecated "register" storage qualifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

add_library(mir-protected-test-framework OBJECT

  fake_input_server_configuration.cpp
  input_testing_server_options.cpp
)

add_library(mir-libinput-test-framework OBJECT libinput_environment.cpp)
add_library(mir-umock-test-framework OBJECT udev_environment.cpp)


add_library(mir-test-framework-static STATIC
  $<TARGET_OBJECTS:mir-public-test-framework>
  $<TARGET_OBJECTS:mir-protected-test-framework>
  $<TARGET_OBJECTS:mir-public-test>
)

add_dependencies(mir-test-framework-static GMock)

target_link_libraries(mir-test-framework-static

  mirclient
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${UMOCKDEV_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-client.map)

add_library(
  mirclientplatformstub MODULE

  stub_client_platform_module.cpp
)

target_link_libraries(
  mirclientplatformstub

  mir-test-framework-static
  mircommon
  ${UMOCKDEV_LDFLAGS} ${UMOCKDEV_LIBRARIES}
)

set_target_properties(
  mirclientplatformstub PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/client-modules
  OUTPUT_NAME dummy
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

add_library(
  mir-test-input-framework OBJECT
  
  stub_input.cpp
  fake_input_device_impl.cpp
  stub_input_platform.cpp
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

install(TARGETS mirplatforminputstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

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
  ${UMOCKDEV_LDFLAGS} ${UMOCKDEV_LIBRARIES}
)

set_target_properties(
  mirplatformgraphicsstub PROPERTIES;
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME graphics-dummy
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${server_symbol_map}"
)

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

install(TARGETS mirplatformgraphicsstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
install(TARGETS mirplatformgraphicsthrow LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

install(TARGETS mirclientplatformstub LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})

add_subdirectory(udev-recordings/)
add_subdirectory(testing-cursor-theme/)
