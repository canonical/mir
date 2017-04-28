include_directories(
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

add_library(
  miroffscreengraphics OBJECT

  display.cpp
  display_configuration.cpp
  display_buffer.cpp
)

