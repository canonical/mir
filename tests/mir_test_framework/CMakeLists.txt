include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${Boost_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
  ${UMOCKDEV_INCLUDE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
)

add_definitions(
    -DMIR_CLIENT_PLATFORM_PATH="${MIR_CLIENT_PLATFORM_PATH}"
    -DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}"
    -DMIR_CLIENT_PLATFORM_ABI_STRING="${MIR_CLIENT_PLATFORM_ABI}"
    -DMIR_SERVER_GRAPHICS_PLATFORM_ABI_STRING="${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    )

add_library(mir-public-test-framework OBJECT
  any_surface.cpp
  async_server_runner.cpp
  command_line_server_configuration.cpp
  connected_client_headless_server.cpp
  connected_client_with_a_surface.cpp
  declarative_placement_window_manage_policy.cpp
  executable_path.cpp
  headless_in_process_server.cpp
  headless_nested_server_runner.cpp
  headless_test.cpp
  interprocess_client_server_test.cpp
  main.cpp
  placement_applying_shell.cpp
  process.cpp
  server_runner.cpp
  socket_detect_server.cpp
  stub_client_platform_factory.cpp
  stub_server_platform_factory.cpp
  stubbed_server_configuration.cpp
  testing_client_options.cpp
  testing_server_options.cpp
  temporary_environment_value.cpp
  using_stub_client_platform.cpp
)

# Umockdev uses glib, which uses the deprecated "register" storage qualifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dregister=")

add_library(mir-protected-test-framework OBJECT

  fake_input_server_configuration.cpp
  input_testing_server_options.cpp
  testing_process_manager.cpp
  udev_environment.cpp
)


add_library(mir-test-framework-static STATIC
  $<TARGET_OBJECTS:mir-public-test-framework>
  $<TARGET_OBJECTS:mir-protected-test-framework>
)

add_dependencies(mir-test-framework-static GMock)

uses_android_input(mir-protected-test-framework)

target_link_libraries(mir-test-framework-static

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
  mirplatforminputstub MODULE
  
  stub_input.cpp
  fake_input_device_impl.cpp
  stub_input_platform.cpp
  $<TARGET_OBJECTS:mirevdevutilsobjects>
  )

target_link_libraries(mirplatforminputstub mircommon)

set_target_properties(
  mirplatforminputstub PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME input-stub
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/symbols-stub-input.map"
)

install(TARGETS mirplatforminputstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

set(server_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-server.map)
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

add_custom_command(TARGET mir-test-framework-static POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/udev_recordings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/udev_recordings
  COMMENT "Copying umockdev recordings to build dir..."
)
add_custom_command(TARGET mir-test-framework-static POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/testing-cursor-theme ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testing-cursor-theme
  COMMENT "Copying testing cursor themes to build dir..."
)

string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

install(TARGETS mirplatformgraphicsstub LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

install(TARGETS mirclientplatformstub LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
