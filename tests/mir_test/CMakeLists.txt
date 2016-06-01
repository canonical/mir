add_library(mir-public-test OBJECT
  cross_process_action.cpp
  cross_process_sync.cpp
  current_thread_name.cpp
  display_config_matchers.cpp
  event_factory.cpp
  event_matchers.cpp
  pipe.cpp
  popen.cpp
  signal.cpp
  spin_wait.cpp
  validity_matchers.cpp
)

add_library(mir-test-static STATIC
  fake_clock.cpp
  fd_utils.cpp
  test_dispatchable.cpp
  wait_object.cpp
  $<TARGET_OBJECTS:mir-public-test>
)
