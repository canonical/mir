# We can't tell which version of gtest we're building against and INSTANTIATE_TEST_SUITE_P changed in
# a way that relies on a gcc extension to support backward-compatible code, So...
check_cxx_compiler_flag(-Wno-gnu-zero-variadic-macro-arguments MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
check_cxx_compiler_flag(-Wno-pedantic MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
if ("${CMAKE_CXX_COMPILER}" MATCHES ".*clang.*" AND MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments") # clang
elseif(MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic") #gcc
endif()

include_directories(
    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
)

string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_NO_LTO_FLAGS})
string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NO_NO_LTO_FLAGS})

if (MIR_BUILD_INTERPROCESS_TESTS)
    list(APPEND MIRAL_TEST_SOURCES window_info.cpp)
endif()

add_compile_definitions(MIRAL_ENABLE_DEPRECATIONS=0)

mir_add_wrapped_executable(miral-test-internal NOINSTALL
    render_scene_into_surface.cpp
    mru_window_list.cpp
    active_outputs.cpp
    command_line_option.cpp
    select_active_window.cpp
    popup_window_placement.cpp
    window_placement_anchors_to_parent.cpp
    drag_active_window.cpp
    modify_window_state.cpp
    modify_window_specification.cpp
    display_reconfiguration.cpp
    display_configuration.cpp
    raise_tree.cpp
    static_display_config.cpp
    test_window_manager_tools.cpp           test_window_manager_tools.h
    depth_layer.cpp
    output_updates.cpp
    application_zone.cpp
    initial_window_placement.cpp
    window_placement_attached.cpp
    window_placement_fullscreen.cpp
    window_placement_maximized.cpp
    window_placement_output.cpp
    resize_and_move.cpp
    ignored_requests.cpp
    focus_mode.cpp
    fd_manager.cpp
    application_selector.cpp
    test_input_configuration.cpp
    test_bounce_keys.cpp
    test_mousekeys_config.cpp
    test_locate_pointer.cpp
    ${MIRAL_TEST_SOURCES}
)

set_source_files_properties(static_display_config.cpp PROPERTIES COMPILE_FLAGS
    "${CMAKE_CXXFLAGS} -I ${PROJECT_SOURCE_DIR}/src/include/common")

target_include_directories(miral-test-internal
    PRIVATE ${PROJECT_SOURCE_DIR}/src/miral ${PROJECT_SOURCE_DIR}/tests/include)

target_link_libraries(miral-test-internal
    miral-internal
    mir-test-assist
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

mir_add_wrapped_executable(miral-test NOINSTALL
    external_client.cpp
    config_file.cpp
    live_config.cpp
    live_config_ini_file.cpp
    magnifier.cpp
    sticky_keys.cpp
    runner.cpp
    wayland_extensions.cpp
    zone.cpp
    server_example_decoration.cpp server_example_decoration.h
    org_kde_kwin_server_decoration.c org_kde_kwin_server_decoration.h
)

mir_generate_protocol_wrapper(miral-test "org_kde_kwin_" server-decoration.xml)

set_source_files_properties(server_example_decoration.cpp PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(miral-test
    mir-test-assist
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::UUID
)

option(MIR_RUN_MIRAL_TESTS "Run miral tests as part of testsuite" ON)

if(MIR_RUN_MIRAL_TESTS)
    set(MIR_EXCLUDE_TESTS
      TestConfigFile.*
    )
    mir_discover_tests_with_fd_leak_detection(miral-test)
    
    set(MIR_EXCLUDE_TESTS)
    mir_discover_tests_with_fd_leak_detection(miral-test-internal)
endif()
