set(
  XWAYLAND_SOURCES

  xwayland_default_configuration.cpp
  xwayland_connector.cpp  xwayland_connector.h
  xwayland_server.cpp     xwayland_server.h
  xcb_atoms.cpp           xcb_atoms.h
  xwayland_wm.cpp         xwayland_wm.h
  xwayland_wm_surface.cpp xwayland_wm_surface.h
  xwayland_wm_shellsurface.cpp xwayland_wm_shellsurface.h
  xwayland_wm_shell.cpp xwayland_wm_shell.h
)

add_definitions(-DMIR_LOG_COMPONENT="xwayland")
include_directories(../frontend_wayland)

# One day, maybe, we can add include dependences to an OBJECT library. Until then...
get_property(mirwayland_includes TARGET mirwayland PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${mirwayland_includes})

add_library(mirfrontend-xwayland OBJECT

  ${XWAYLAND_SOURCES}
)
