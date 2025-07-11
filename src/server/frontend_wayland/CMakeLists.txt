include_directories(../frontend_xwayland ${CMAKE_CURRENT_BINARY_DIR})

# One day, maybe, we can add include dependences to an OBJECT library. Until then...
get_property(mirwayland_includes TARGET mirwayland PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${mirwayland_includes})
pkg_check_modules(GIOUnix REQUIRED IMPORTED_TARGET gio-unix-2.0)

set(
  WAYLAND_SOURCES

  wayland_default_configuration.cpp
  wayland_connector.cpp         wayland_connector.h
  wl_client.cpp                 wl_client.h
  wayland_executor.cpp          wayland_executor.h
  wayland_surface_observer.cpp  wayland_surface_observer.h
  wayland_input_dispatcher.cpp  wayland_input_dispatcher.h
  wl_data_device_manager.cpp    wl_data_device_manager.h
  wl_data_device.cpp            wl_data_device.h
  wl_data_source.cpp            wl_data_source.h
  output_manager.cpp            output_manager.h
  pointer_constraints_unstable_v1.cpp pointer_constraints_unstable_v1.h
  relative_pointer_unstable_v1.cpp    relative_pointer_unstable_v1.h
  wl_subcompositor.cpp          wl_subcompositor.h
                                wl_surface_role.h
  window_wl_surface_role.cpp    window_wl_surface_role.h
  wl_surface.cpp                wl_surface.h
  wl_seat.cpp                   wl_seat.h
  keyboard_helper.cpp           keyboard_helper.h
  wl_keyboard.cpp               wl_keyboard.h
  wl_pointer.cpp                wl_pointer.h
  wl_touch.cpp                  wl_touch.h
  wl_shell.cpp                  wl_shell.h
  xdg_shell_v6.cpp              xdg_shell_v6.h
  xdg_shell_stable.cpp          xdg_shell_stable.h
  xdg_output_v1.cpp             xdg_output_v1.h
  layer_shell_v1.cpp            layer_shell_v1.h
  mir_shell.cpp                 mir_shell.h
  resource_lifetime_tracker.cpp resource_lifetime_tracker.h
  wl_region.cpp                 wl_region.h
  foreign_toplevel_manager_v1.cpp foreign_toplevel_manager_v1.h
  desktop_file_manager.cpp      desktop_file_manager.h
  frame_executor.cpp            frame_executor.h
  virtual_keyboard_v1.cpp       virtual_keyboard_v1.h
  virtual_pointer_v1.cpp        virtual_pointer_v1.h
  text_input_v3.cpp             text_input_v3.cpp
  text_input_v2.cpp             text_input_v2.cpp
  input_method_common.cpp       input_method_common.h
  input_method_v1.cpp           input_method_v1.h
  input_method_v2.cpp           input_method_v2.h
  input_method_grab_keyboard_v2.cpp input_method_grab_keyboard_v2.h
  idle_inhibit_v1.cpp           idle_inhibit_v1.h
  wlr_screencopy_v1.cpp         wlr_screencopy_v1.h
  text_input_v1.cpp             text_input_v1.h
  primary_selection_v1.cpp      primary_selection_v1.h
  session_lock_v1.cpp           session_lock_v1.h
  xdg_decoration_unstable_v1.cpp           xdg_decoration_unstable_v1.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/wayland.h
  ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.c
  ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.h
  std_layout_uptr.h
  shm.cpp                       shm.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/pointer_input_dispatcher.h
  session_credentials.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/buffer_stream.h
  wp_viewporter.cpp             wp_viewporter.h
  fractional_scale_v1.cpp           fractional_scale_v1.h
  xdg_activation_v1.cpp         xdg_activation_v1.h
  linux_drm_syncobj.cpp         linux_drm_syncobj.h
)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.c
    ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.h
  COMMAND
    lttng-gen-tp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_frontend.tp
        -o wayland_frontend.tp.h
        -o wayland_frontend.tp.c
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/wayland_frontend.tp
)

# Inconveniently, GCC on 16.04 hits an ICE when attempting to use LTO
# on the tracepoints. Fortunately we can turn it off for just that translation
# unit.
if (CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6))
  set(TRACEPOINT_COMPILE_FLAGS "-fno-lto")
endif()

check_cxx_compiler_flag(-Wgnu-empty-initializer HAS_W_GNU_EMPTY_INITIALIZER)
if (HAS_W_GNU_EMPTY_INITIALIZER)
  set(TRACEPOINT_COMPILE_FLAGS "${TRACEPOINT_COMPILE_FLAGS} -Wno-error=gnu-empty-initializer")
endif()

set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.c
        ${CMAKE_CURRENT_BINARY_DIR}/wayland_frontend.tp.h
        PROPERTIES
        COMPILE_FLAGS "${TRACEPOINT_COMPILE_FLAGS}"
)

add_library(mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

target_link_libraries(mirfrontend-wayland
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::GIOUnix
)

