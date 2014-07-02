list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_input_dispatcher_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_communication_package.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_application_handle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_window_handle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_targeter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_sender.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_target_enumerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_registrar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_input_translator.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
