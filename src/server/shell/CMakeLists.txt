set(
  SHELL_SOURCES

  application_session.cpp
  default_session_container.cpp
  session_manager.cpp
  registration_order_focus_sequence.cpp
  single_visibility_focus_mechanism.cpp
  consuming_placement_strategy.cpp
  organising_surface_factory.cpp
  surface_source.cpp
  surface.cpp
  surface_creation_parameters.cpp
)

add_library(
  mirshell STATIC

  ${SHELL_SOURCES}
)
