list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_display_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_persistent_surface_store_id.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_persistent_surface_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_decoration_basic_manager.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
