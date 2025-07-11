include_directories(${PROJECT_SOURCE_DIR}/include/renderers/sw)
set(
  INPUT_SOURCES

  basic_seat.cpp
  builtin_cursor_images.cpp
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
  keyboard_resync_dispatcher.cpp
  null_input_dispatcher.cpp
  seat_input_device_tracker.cpp
  surface_input_dispatcher.cpp
  touchspot_controller.cpp
  validator.cpp
  vt_filter.cpp
  seat_observer_multiplexer.cpp
  seat_observer_multiplexer.h
  idle_poking_dispatcher.cpp
  virtual_input_device.cpp
  xkb_mapper_registrar.cpp
  input_event_transformer.cpp
  cursor_observer_multiplexer.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/seat_observer.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/input_dispatcher.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/seat.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/input_probe.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/input_event_transformer.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/input/cursor_observer_multiplexer.h
)

set_property(
    SOURCE default_configuration.cpp
    PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

add_library(
  mirinput OBJECT

  ${INPUT_SOURCES}
)

target_link_libraries(mirinput
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::GLIB
)
