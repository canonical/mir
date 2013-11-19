add_library(
  miroffscreengraphics STATIC

  display.cpp
  display_configuration.cpp
  display_buffer.cpp
  surfaceless_egl_context.cpp
  gl_extensions_base.cpp
)

target_link_libraries(
  miroffscreengraphics
)
