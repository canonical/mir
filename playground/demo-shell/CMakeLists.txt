add_library(demo-shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  window_manager.cpp
)

mir_add_wrapped_executable(mir_proving_server
  demo_shell.cpp
)

target_link_libraries(mir_proving_server
  demo-shell
  mirserver
  playgroundserverconfig
  exampleserverconfig
)
