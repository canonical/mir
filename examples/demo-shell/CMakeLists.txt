add_executable(mir_demo_serve_shell
  demo_shell.cpp
  fullscreen_placement_strategy.cpp
  application_switcher.cpp
)

target_link_libraries(mir_demo_serve_shell
  mirserver
)

install(TARGETS mir_demo_serve_shell RUNTIME DESTINATION bin)
