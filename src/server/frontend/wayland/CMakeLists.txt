set(
  WAYLAND_SOURCES

  core_generated_interfaces.h
  xdg_shell_generated_interfaces.h
  xdg-shell-unstable-v6.h
  xdg-shell-unstable-v6.c
  wayland_default_configuration.cpp
  wayland_connector.cpp wayland_connector.h
  wlshmbuffer.cpp       wlshmbuffer.h)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

