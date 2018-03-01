add_subdirectory(protocol/)
add_subdirectory(generated/)

set(
  WAYLAND_SOURCES

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
  wl_keyboard.cpp               wl_keyboard.h
  wl_pointer.cpp                wl_pointer.h
  wl_touch.cpp                  wl_touch.h
  xdg_shell_base.cpp            xdg_shell_base.h
  xdg_shell_stable.cpp          xdg_shell_stable.h
  xdg_shell_unstable_v6.cpp     xdg_shell_unstable_v6.h
)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

