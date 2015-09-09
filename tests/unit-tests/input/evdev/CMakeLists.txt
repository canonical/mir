list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_evdev_device_detection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_evdev_input_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_libinput_device.cpp
  $<TARGET_OBJECTS:mirevdevutilsobjects>
  $<TARGET_OBJECTS:mirplatforminputevdevobjects>
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
