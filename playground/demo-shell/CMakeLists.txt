add_library(demo-shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  fullscreen_placement_strategy.cpp
  window_manager.cpp
)

mir_add_wrapped_executable(mir_demo_server_shell
  demo_shell.cpp
)

target_link_libraries(mir_demo_server_shell
  demo-shell
  mirserver
  playgroundserverconfig
  exampleserverconfig
)
