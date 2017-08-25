set(
  WAYLAND_SOURCES

  wayland_default_configuration.cpp
  wayland_connector.cpp
)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

add_dependencies(mirfrontend-wayland wayland-headers)
