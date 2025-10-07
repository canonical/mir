set(
  XWAYLAND_SOURCES

  xwayland_default_configuration.cpp
  xwayland_connector.cpp  xwayland_connector.h
  xwayland_spawner.cpp    xwayland_spawner.h
  xwayland_server.cpp     xwayland_server.h
  xcb_connection.cpp      xcb_connection.h
  xwayland_wm.cpp         xwayland_wm.h
  xwayland_cursors.cpp    xwayland_cursors.h
  xwayland_clipboard_provider.cpp xwayland_clipboard_provider.h
  xwayland_clipboard_source.cpp xwayland_clipboard_source.h
  xwayland_surface.cpp    xwayland_surface.h
  xwayland_client_manager.cpp xwayland_client_manager.h
  xwayland_surface_role.cpp xwayland_surface_role.h
                          xwayland_surface_role_surface.h
  xwayland_surface_observer.cpp xwayland_surface_observer.h
                          xwayland_surface_observer_surface.h
  xwayland_wm_shell.h
  scaled_buffer_stream.cpp scaled_buffer_stream.h
)

add_compile_definitions(MIR_LOG_COMPONENT="xwayland")
include_directories(../frontend_wayland)

# One day, maybe, we can add include dependences to an OBJECT library. Until then...
get_property(mirwayland_includes TARGET mirwayland PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${mirwayland_includes})

add_library(mirfrontend-xwayland OBJECT

  ${XWAYLAND_SOURCES}
)

target_link_libraries(mirfrontend-xwayland
  PUBLIC
    mirplatform
    mircommon
    mircore
)
