list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_wayland_executor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_wayland_weak.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_lifetime_tracker.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
