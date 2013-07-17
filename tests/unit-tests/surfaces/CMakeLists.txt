list(
  APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_data.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_stack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_controller.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
