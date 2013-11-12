include_directories(..)

add_executable(mir_demo_standalone_inprocess_egl
  demo_inprocess_surface_client.cpp
  inprocess_egl_client.cpp
  example_egl_helper.cpp
  ../server_configuration.cpp
)

target_link_libraries(mir_demo_standalone_inprocess_egl
  mirserver
  mirdraw
)
# TODO Install this demo?
