set(
  SHELL_SOURCES

  abstract_shell.cpp
  canonical_window_manager.cpp
  default_placement_strategy.cpp
  frontend_shell.cpp
  graphics_display_layout.cpp
  default_configuration.cpp
  default_window_manager.cpp
  shell_wrapper.cpp
  surface_ready_observer.cpp
  default_persistent_surface_store.cpp
  persistent_surface_store.cpp
)

add_library(
  mirshell OBJECT

  ${SHELL_SOURCES}
)
