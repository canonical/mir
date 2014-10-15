list(
  APPEND INPUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_channel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rudimentary_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event_filter_dispatcher_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_application_handle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_window_handle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_registrar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_targeter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_target_enumerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common_input_thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_channel_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_translator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_sender.cpp
)

set(
  INPUT_SOURCES
  ${INPUT_SOURCES}
  PARENT_SCOPE
)

