add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="demo_server")

add_library(exampleserverconfig STATIC
  server_example_input_event_filter.cpp
  server_example_input_filter.cpp
)

target_include_directories(exampleserverconfig
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(exampleserverconfig
  mirserver
  mirwayland
)

set(LAYER_SHELL_H "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1.h")
set(LAYER_SHELL_C "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1.c")
set(LAYER_SHELL_X "${PROJECT_SOURCE_DIR}/wayland-protocols/wlr-layer-shell-unstable-v1.xml")

add_custom_command(
    OUTPUT "${LAYER_SHELL_H}" "${LAYER_SHELL_C}"
    VERBATIM
    COMMAND "sh" "-c" "wayland-scanner client-header ${LAYER_SHELL_X} ${LAYER_SHELL_H}"
    COMMAND "sh" "-c" "wayland-scanner private-code  ${LAYER_SHELL_X} ${LAYER_SHELL_C}"
    # The protocol uses "namespace" as an argument name, which is a C++ keyword
    COMMAND sed -i s/\\<namespace\\>/namespace_/g "${LAYER_SHELL_H}"
    DEPENDS "${LAYER_SHELL_X}"
)

mir_add_wrapped_executable(mir_demo_server
    server_example.cpp
    server_example_background.cpp
    server_example_test_client.cpp
    ${LAYER_SHELL_C}
    ${LAYER_SHELL_H}
)

mir_disable_tsan_for_sources("${LAYER_SHELL_C}")

target_include_directories(mir_demo_server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mir_demo_server
  example-shell-lib
  exampleserverconfig
  PkgConfig::WAYLAND_CLIENT
)
