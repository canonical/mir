add_library(
  mirfrontend-waylandgenerated OBJECT

  wayland_wrapper.cpp                       wayland_wrapper.h
  xdg-shell-unstable-v6_wrapper.cpp         xdg-shell-unstable-v6_wrapper.h
  xdg-shell_wrapper.cpp                     xdg-shell_wrapper.h
  wlr-layer-shell-unstable-v1_wrapper.cpp   wlr-layer-shell-unstable-v1_wrapper.h
)
