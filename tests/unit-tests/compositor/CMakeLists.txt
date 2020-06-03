list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_buffer_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_threaded_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_occlusion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_monitor_arbiter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_dropping_schedule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_queueing_schedule.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
