include_directories(${PROJECT_SOURCE_DIR}/include/renderers/sw)
set(
  INPUT_SOURCES

  basic_seat.cpp
  builtin_cursor_images.cpp
  config_changer.cpp
  config_changer.h
  cursor_controller.cpp
  default_configuration.cpp
  default_device.cpp
  default_event_builder.cpp
  default_input_device_hub.cpp
  default_input_manager.cpp
  event_filter_chain_dispatcher.cpp
  input_modifier_utils.cpp
  input_probe.cpp
  key_repeat_dispatcher.cpp
  null_input_dispatcher.cpp
  seat_input_device_tracker.cpp
  surface_input_dispatcher.cpp
  touchspot_controller.cpp
  validator.cpp
  vt_filter.cpp
  seat_observer_multiplexer.cpp
  seat_observer_multiplexer.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/input/seat_observer.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/input/input_dispatcher.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/seat.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/input_probe.h
)

add_library(
  mirinput OBJECT

  ${INPUT_SOURCES}
)
