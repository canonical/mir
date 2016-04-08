include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

set(
  INPUT_SOURCES

  basic_seat.cpp
  builtin_cursor_images.cpp
  channel.cpp
  channel_factory.cpp
  cursor_controller.cpp
  default_configuration.cpp
  default_device.cpp
  default_event_builder.cpp
  default_input_device_hub.cpp
  default_input_manager.cpp
  display_input_region.cpp
  event_filter_chain_dispatcher.cpp
  input_modifier_utils.cpp
  input_probe.cpp
  key_repeat_dispatcher.cpp
  null_input_channel_factory.cpp
  null_input_dispatcher.cpp
  seat_input_device_tracker.cpp
  surface_input_dispatcher.cpp
  touchspot_controller.cpp
  validator.cpp
  vt_filter.cpp
)

add_subdirectory(android)

add_library(
  mirinput OBJECT

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
