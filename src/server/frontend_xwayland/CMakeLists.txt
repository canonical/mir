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

add_library(mirfrontend-xwayland OBJECT

  ${XWAYLAND_SOURCES}
)

target_include_directories(mirfrontend-xwayland
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
    ../frontend_wayland
)

target_link_libraries(mirfrontend-xwayland
  PUBLIC
    mirplatform
    mircommon
    mircore
)
