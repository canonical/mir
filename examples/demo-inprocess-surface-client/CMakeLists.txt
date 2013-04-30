include_directories(..)

add_executable(mir_demo_inprocess_egl
  demo_inprocess_surface_client.cpp
  inprocess_egl_client.cpp
  example_egl_helper.cpp
)

target_link_libraries(mir_demo_inprocess_egl
  mirserver
  mirdraw
)
# TODO Install this demo?
