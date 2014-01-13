add_library(
  miroffscreengraphics STATIC

  display.cpp
  display_configuration.cpp
  display_buffer.cpp
)

target_link_libraries(
  miroffscreengraphics
)
