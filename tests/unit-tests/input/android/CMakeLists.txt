list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_channel_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_communication_package.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_sender.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
