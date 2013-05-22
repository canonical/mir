add_executable(mir_demo_server_shell
  demo_shell.cpp
  fullscreen_placement_strategy.cpp
  application_switcher.cpp
)

target_link_libraries(mir_demo_server_shell
  mirserver
)

install(TARGETS mir_demo_server_shell RUNTIME DESTINATION bin)
