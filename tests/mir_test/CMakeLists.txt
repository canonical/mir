add_library(
  mir-test STATIC

  pipe.cpp
  display_config_matchers.cpp
  cross_process_action.cpp
  spin_wait.cpp
  signal.cpp
  fake_clock.cpp
  popen.cpp
  wait_object.cpp
  current_thread_name.cpp
)

add_dependencies(mir-test mirprotobuf)
