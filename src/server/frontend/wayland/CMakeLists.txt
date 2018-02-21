set(
  WAYLAND_SOURCES

  core_generated_interfaces.h
  xdg_shell_generated_interfaces.h
  xdg-shell-unstable-v6.h
  xdg-shell-unstable-v6.c
  wayland_default_configuration.cpp
  wayland_connector.cpp         wayland_connector.h
  wlshmbuffer.cpp               wlshmbuffer.h
  wayland_executor.cpp          wayland_executor.h
  null_event_sink.cpp           null_event_sink.h
  basic_surface_event_sink.cpp  basic_surface_event_sink.h
  output_manager.cpp            output_manager.h
  wl_mir_window.cpp             wl_mir_window.h
  wl_surface.cpp                wl_surface.h
  wl_seat.cpp                   wl_seat.h
  wl_keybaord.cpp               wl_keybaord.h
)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

