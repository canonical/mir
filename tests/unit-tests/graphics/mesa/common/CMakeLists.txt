add_library(unit_test_graphics_mesa_common OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/test_anonymous_shm_file.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_drm_helper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ipc_operations.cpp
)

list(APPEND UNIT_TEST_SOURCES
  $<TARGET_OBJECTS:unit_test_graphics_mesa_common>
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
