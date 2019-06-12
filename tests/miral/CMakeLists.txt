# We can't tell which version of gtest we're building against and INSTANTIATE_TEST_CASE_P changed in
# a way that relies on a gcc extension to support backward-compatible code, So...
check_cxx_compiler_flag(-Wno-gnu-zero-variadic-macro-arguments MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
check_cxx_compiler_flag(-Wno-pedantic MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
if ("${CMAKE_CXX_COMPILER}" MATCHES ".*clang.*" AND MIRAL_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments") # clang
elseif(MIRAL_COMPILE_WITH_W_NO_PEDANTIC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic") #gcc
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/src/miral
    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
)

string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_NO_LTO_FLAGS})
string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NO_NO_LTO_FLAGS})

add_library(miral-test-framework STATIC
    test_server.cpp     ../include/miral/test_server.h
)

target_include_directories(miral-test-framework PUBLIC ../include/miral)

target_link_libraries(miral-test-framework
PUBLIC
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARIES}
    miral
    mir-test-assist
PRIVATE
    miral-internal
)

mir_add_wrapped_executable(miral-test-internal NOINSTALL
    mru_window_list.cpp
    active_outputs.cpp
    command_line_option.cpp
    select_active_window.cpp
    window_placement.cpp
    window_placement_anchors_to_parent.cpp
    drag_active_window.cpp
    modify_window_state.cpp
    modify_window_specification.cpp
    display_reconfiguration.cpp
    raise_tree.cpp
    static_display_config.cpp
    client_mediated_gestures.cpp
    window_info.cpp
    test_window_manager_tools.cpp           test_window_manager_tools.h
    depth_layer.cpp
    output_updates.cpp
)

set_source_files_properties(static_display_config.cpp PROPERTIES COMPILE_FLAGS
    "${CMAKE_CXXFLAGS} -I ${PROJECT_SOURCE_DIR}/src/include/common")


target_link_libraries(miral-test-internal
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARIES}
    miral-internal
    mir-test-assist
)

add_subdirectory(generated/)

mir_add_wrapped_executable(miral-test NOINSTALL
    window_id.cpp
    runner.cpp
    window_placement_client_api.cpp
    window_properties.cpp
    active_window.cpp
    wayland_extensions.cpp
    workspaces.cpp
    drag_and_drop.cpp
    server_example_decoration.cpp server_example_decoration.h
    org_kde_kwin_server_decoration.c org_kde_kwin_server_decoration.h
    generated/server-decoration_wrapper.cpp generated/server-decoration_wrapper.h
)

set_source_files_properties(server_example_decoration.cpp PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/generated/")

target_link_libraries(miral-test
    miral-test-framework
    ${UUID_LDFLAGS} ${UUID_LIBRARIES}
)

mir_discover_tests_with_fd_leak_detection(miral-test)
mir_discover_tests_with_fd_leak_detection(miral-test-internal)
