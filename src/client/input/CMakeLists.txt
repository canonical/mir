list(
  APPEND CLIENT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_receiver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_receiver_thread.cpp
)

set(
  CLIENT_SOURCES
  ${CLIENT_SOURCES}
  PARENT_SCOPE
)