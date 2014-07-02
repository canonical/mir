set(
  SHELL_SOURCES

  default_focus_mechanism.cpp
  consuming_placement_strategy.cpp
  graphics_display_layout.cpp
  default_configuration.cpp
  session_coordinator_wrapper.cpp
  surface_coordinator_wrapper.cpp
)

add_library(
  mirshell STATIC

  ${SHELL_SOURCES}
)
