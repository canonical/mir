add_library(
  mirfrontend-waylandgenerated OBJECT

  wayland.c                 wayland.h               wayland_wrapper.h
  xdg-shell-unstable-v6.c   xdg-shell-unstable-v6.h xdg-shell-unstable-v6_wrapper.h
)
