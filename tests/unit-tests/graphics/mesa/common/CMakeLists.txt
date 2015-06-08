list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_anonymous_shm_file.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_drm_helper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ipc_operations.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
