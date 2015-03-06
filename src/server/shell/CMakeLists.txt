set(
  SHELL_SOURCES

  abstract_shell.cpp
  default_placement_strategy.cpp
  frontend_shell.cpp
  generic_shell.cpp
  graphics_display_layout.cpp
  default_configuration.cpp
  default_shell.cpp
  shell_wrapper.cpp
)

add_library(
  mirshell OBJECT

  ${SHELL_SOURCES}
)
