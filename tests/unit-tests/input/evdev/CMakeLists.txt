list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_evdev_device_detection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_libinput_device.cpp
)
list(APPEND UNIT_TEST_REFERENCES
  mirevdevutilsobjects
  mirplatforminputevdevobjects
)

list(APPEND UMOCK_UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_evdev_input_platform.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)

set(
  UNIT_TEST_REFERENCES
  ${UNIT_TEST_REFERENCES}
  PARENT_SCOPE)

set(
  UMOCK_UNIT_TEST_SOURCES
  ${UMOCK_UNIT_TEST_SOURCES}
  PARENT_SCOPE
)
