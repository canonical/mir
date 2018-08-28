add_subdirectory(generator/)
add_subdirectory(generated/)

include_directories(../frontend_xwayland)

set(
  WAYLAND_SOURCES

  wayland_default_configuration.cpp
  wayland_connector.cpp         wayland_connector.h
  wlshmbuffer.cpp               wlshmbuffer.h
  wayland_executor.cpp          wayland_executor.h
  null_event_sink.cpp           null_event_sink.h
  wl_surface_event_sink.cpp     wl_surface_event_sink.h
  data_device.cpp               data_device.h
  output_manager.cpp            output_manager.h
  wl_subcompositor.cpp          wl_subcompositor.h
                                wl_surface_role.h
  window_wl_surface_role.cpp    window_wl_surface_role.h
  wl_surface.cpp                wl_surface.h
  wl_seat.cpp                   wl_seat.h
  wl_keyboard.cpp               wl_keyboard.h
  wl_pointer.cpp                wl_pointer.h
  wl_touch.cpp                  wl_touch.h
  xdg_shell_v6.cpp              xdg_shell_v6.h
  xdg_shell_stable.cpp          xdg_shell_stable.h
  deleted_for_resource.cpp      deleted_for_resource.h
  wl_region.cpp                 wl_region.h)

add_library(
  mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)

add_dependencies(
  mirfrontend-wayland
  mirfrontend-waylandgenerated
)
