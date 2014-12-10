add_library(demo-shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  window_manager.cpp
)

add_executable(mir_demo_server_shell
  demo_shell.cpp
)

target_link_libraries(mir_demo_server_shell
  demo-shell
  mirserver
  playgroundserverconfig
  exampleserverconfig
)

install(TARGETS mir_demo_server_shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
