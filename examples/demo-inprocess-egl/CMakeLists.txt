add_executable(mir_demo_inprocess_egl
  demo_inprocess_egl.cpp
)

target_link_libraries(mir_demo_inprocess_egl
  mirserver
)
# TODO Install this demo?