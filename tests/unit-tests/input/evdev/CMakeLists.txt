list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_evdev_device_detection.cpp
  $<TARGET_OBJECTS:mirevdevutilsobjects>
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
