list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_buffer_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_temporary_buffers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_threaded_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_program_family.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_texture_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_occlusion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_screencast_display_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_compositing_screencast.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_timeout_frame_dropping_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_monitor_arbiter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_buffers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_dropping_schedule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_queueing_schedule.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
