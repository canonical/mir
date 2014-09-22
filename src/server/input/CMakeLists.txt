include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
include_directories(${MIR_XCURSOR_INCLUDE_DIRECTORIES})

set(
  INPUT_SOURCES

  event_filter_chain.cpp

  null_input_channel_factory.cpp
  null_input_configuration.cpp
  null_input_dispatcher.cpp
  display_input_region.cpp
  vt_filter.cpp
  cursor_controller.cpp
  default_configuration.cpp
  xcursor_loader.cpp
  builtin_cursor_images.cpp
  touchspot_controller.cpp
)

add_subdirectory(android)

add_library(
  mirinput OBJECT

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
