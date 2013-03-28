
if (NOT MIR_DISABLE_INPUT)
list(
  APPEND CLIENT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_receiver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_receiver_thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_platform.cpp
)
else()
list(
  APPEND CLIENT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/dummy_input_platform.cpp
)
endif()

set(
  CLIENT_SOURCES
  ${CLIENT_SOURCES}
  PARENT_SCOPE
)