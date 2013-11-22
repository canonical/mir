list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display_multi_monitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_real_kms_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_page_flipper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_native_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_native_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_udev_helper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_overlapping_output_grouping.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_native_gbm_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_anonymous_shm_file.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
