list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_input_dispatcher_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_communication_package.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
