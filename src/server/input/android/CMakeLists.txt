list(
  APPEND INPUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_channel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rudimentary_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_channel_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_reader_dispatchable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_translator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/input_sender.cpp
)

set(
  INPUT_SOURCES
  ${INPUT_SOURCES}
  PARENT_SCOPE
)

