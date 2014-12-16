add_library(demo-shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  window_manager.cpp
)

add_executable(mir_proving_server
  demo_shell.cpp
)

target_link_libraries(mir_proving_server
  demo-shell
  mirserver
  playgroundserverconfig
  exampleserverconfig
)

install(TARGETS mir_proving_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
