set(
  SHELL_SOURCES

  default_focus_mechanism.cpp
  consuming_placement_strategy.cpp
  organising_surface_factory.cpp
  surface_creation_parameters.cpp
  graphics_display_layout.cpp
  default_configuration.cpp
)

add_library(
  mirshell STATIC

  ${SHELL_SOURCES}
)
