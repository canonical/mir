set(
  WAYLAND_SOURCES

  core_generated_interfaces.h
  wayland_default_configuration.cpp
  wayland_connector.cpp wayland_connector.h
  wlshmbuffer.cpp       wlshmbuffer.h)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

