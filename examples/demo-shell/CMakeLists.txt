add_executable(mir_demo_shell
  demo_shell.cpp
  fullscreen_placement_strategy.cpp
  window_manager.cpp
)

target_link_libraries(mir_demo_shell
  mirserver
)

install(TARGETS mir_demo_shell RUNTIME DESTINATION bin)
