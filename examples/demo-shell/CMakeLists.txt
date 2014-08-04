add_library(demo_shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  fullscreen_placement_strategy.cpp
  window_manager.cpp
  ../server_configuration.cpp
)

add_executable(mir_demo_server_shell
  demo_shell.cpp
)

target_link_libraries(mir_demo_server_shell
  demo_shell
  mirserver
)

install(TARGETS mir_demo_server_shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
