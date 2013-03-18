list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_application_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_the_session_container_implementation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_registration_order_focus_sequence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_single_visibility_focus_mechanism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_consuming_placement_strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_organising_surface_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
