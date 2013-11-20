list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_consuming_placement_strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_organising_surface_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_display_layout.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
