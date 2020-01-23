add_subdirectory(evdev)

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_touchspot_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_input_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_config_changer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_builders.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_external_input_device_hub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_input_device_hub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_surface_input_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_seat_input_device_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_key_repeat_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_validator.cpp
)

list(APPEND UMOCK_UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_input_platform_probing.cpp
    $<TARGET_OBJECTS:mirplatforminputevdevobjects>
    $<TARGET_OBJECTS:mirevdevutilsobjects>
)
if (MIR_BUILD_PLATFORM_MESA_X11)
list(APPEND UMOCK_UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_x11_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_x11_module.cpp
    $<TARGET_OBJECTS:mirplatformservermesax11sharedresources>
    $<TARGET_OBJECTS:mirplatforminputmesax11objects>
)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

set(UMOCK_UNIT_TEST_SOURCES ${UMOCK_UNIT_TEST_SOURCES} PARENT_SCOPE)
