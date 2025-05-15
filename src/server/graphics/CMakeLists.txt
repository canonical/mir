include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
)

add_library(
  mirgraphics OBJECT

  default_configuration.cpp
  default_display_configuration_policy.cpp
  software_cursor.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/graphics/display_configuration_observer.h
  display_configuration_observer_multiplexer.cpp
  display_configuration_observer_multiplexer.h
  platform_probe.cpp
  platform_probe.h
  multiplexing_display.h
  multiplexing_display.cpp
  multiplexing_hw_cursor.h
  multiplexing_hw_cursor.cpp
  default_output_filter.h
  default_output_filter.cpp
)

target_link_libraries(mirgraphics
  PUBLIC
    mirplatform
    mircommon
    mircore
)
