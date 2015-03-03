list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_receiver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_xkb_mapper.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
