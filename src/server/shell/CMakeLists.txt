set(
  SHELL_SOURCES

  default_shell_configuration.cpp
  application_session.cpp
  session_container.cpp
  session_manager.cpp
  registration_order_focus_sequence.cpp
  single_visibility_focus_mechanism.cpp
  consuming_placement_strategy.cpp
  organising_surface_factory.cpp
  surface_source.cpp
  surface.cpp
)

add_library(
  mirshell STATIC

  ${SHELL_SOURCES}
)
