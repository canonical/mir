set(
  SHELL_SOURCES

  default_focus_mechanism.cpp
  default_placement_strategy.cpp
  graphics_display_layout.cpp
  default_configuration.cpp
  default_shell.cpp
  session_coordinator_wrapper.cpp
  surface_coordinator_wrapper.cpp
)

add_library(
  mirshell OBJECT

  ${SHELL_SOURCES}
)
