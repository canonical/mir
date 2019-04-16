include_directories(../frontend_xwayland ${CMAKE_CURRENT_SOURCE_DIR})

# One day, maybe, we can add include dependences to an OBJECT library. Until then...
get_property(mirwayland_includes TARGET mirwayland PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${mirwayland_includes})

set(
  WAYLAND_SOURCES

  mir_display.cpp               mir_display.h
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
  layer_shell_v1.cpp            layer_shell_v1.h
  deleted_for_resource.cpp      deleted_for_resource.h
  wl_region.cpp                 wl_region.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/frontend/wayland.h
  wayland_frontend.tp.c         wayland_frontend.tp.h
)

add_library(mirfrontend-wayland OBJECT

  ${WAYLAND_SOURCES}
)
