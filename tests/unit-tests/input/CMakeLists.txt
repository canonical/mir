add_subdirectory(android)
add_subdirectory(evdev)

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_input_region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_xcursor_loader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_touchspot_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_input_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_builders.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_input_device_hub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_input_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_key_repeat_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_validator.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_x11_dispatchable.cpp
  )
endif()

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
