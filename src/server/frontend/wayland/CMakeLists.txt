set(
  WAYLAND_SOURCES

  core_generated_interfaces.h
  wayland_default_configuration.cpp
  wayland_connector.cpp
)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

