mir_add_wrapped_executable(mir_test_client_impolite_shutdown
  scroll.cpp
)

target_link_libraries(mir_test_client_impolite_shutdown
  mirdraw
  mirclient

  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)
