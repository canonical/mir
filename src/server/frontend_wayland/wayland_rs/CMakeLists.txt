add_rust_cxx_library(mirwayland_rs
  CRATE wayland_rs
  CXX_BRIDGE_SOURCE_FILE src/ffi.rs
  DEPENDS Cargo.toml build_script/main.rs build_script/protocol_parser.rs src/lib.rs src/wayland_server_core.rs
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/include
)

set(wayland_rs_generated_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_data_control_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_foreign_toplevel_list_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_image_capture_source_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_image_copy_capture_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_input_trigger_action_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_input_trigger_registration_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/ext_session_lock_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/fractional_scale_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/global_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/idle_inhibit_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/input_method_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/input_method_unstable_v2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/linux_dmabuf_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/linux_drm_syncobj_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/mir_shell_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/pointer_constraints_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/relative_pointer_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/server_decoration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/text_input_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/text_input_unstable_v2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/text_input_unstable_v3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/viewporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/virtual_keyboard_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wayland.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wlr_foreign_toplevel_management_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wlr_layer_shell_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wlr_screencopy_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wlr_virtual_pointer_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/wp_primary_selection_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/xdg_activation_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/xdg_decoration_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/xdg_output_unstable_v1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/xdg_shell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wayland_rs_cpp/src/xdg_shell_unstable_v6.cpp
)

set_source_files_properties(${wayland_rs_generated_sources} PROPERTIES GENERATED TRUE)

target_sources(mirwayland_rs-cxxbridge
  PRIVATE
    ${wayland_rs_generated_sources}
)

add_executable(
  mirwayland_rs-demo
  ${CMAKE_CURRENT_SOURCE_DIR}/example/main.cpp
)

target_include_directories(mirwayland_rs-demo
  PRIVATE
  "$<TARGET_PROPERTY:mirwayland_rs,INTERFACE_INCLUDE_DIRECTORIES>")

target_link_libraries(mirwayland_rs-demo
  PRIVATE
  mirwayland_rs
  PkgConfig::WAYLAND_CLIENT)
