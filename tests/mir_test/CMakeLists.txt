# We don't want LTO objects in a published archive (libmir-test-assist.a)
string (REPLACE " -flto " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string (REPLACE " -flto " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

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

if (NOT HAVE_PTHREAD_GETNAME_NP)
    set_source_files_properties (current_thread_name.cpp PROPERTIES COMPILE_DEFINITIONS MIR_DONT_USE_PTHREAD_GETNAME_NP
    )
    message(WARNING "pthread_getname_np() not supported by libc. Stubbing out mir::test::current_thread_name() that relies on it")
endif()
