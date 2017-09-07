set(
  SHELL_SOURCES

  abstract_shell.cpp
  frontend_shell.cpp
  graphics_display_layout.cpp
  graphics_display_layout.h
  default_configuration.cpp
  null_host_lifecycle_event_listener.h
  shell_wrapper.cpp
  surface_ready_observer.cpp
  system_compositor_window_manager.cpp
  default_persistent_surface_store.cpp
  persistent_surface_store.cpp
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/display_configuration_controller.h
  surface_specification.cpp
  surface_stack_wrapper.cpp ${CMAKE_SOURCE_DIR}/include/server/mir/shell/surface_stack_wrapper.h
  window_management_info.cpp   ${CMAKE_SOURCE_DIR}/include/server/mir/shell/window_management_info.h
)

add_library(
  mirshell OBJECT

  ${SHELL_SOURCES}
)
