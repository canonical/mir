list(
  APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_wayland_timespec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_screencopy_v1_damage_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_file_manager.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
