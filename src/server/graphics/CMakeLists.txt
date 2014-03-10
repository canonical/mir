include_directories(${GLESv2_INCLUDE_DIRS})

add_library(
  mirgraphics STATIC

  default_configuration.cpp
  default_display_configuration_policy.cpp
  gl_extensions_base.cpp
  surfaceless_egl_context.cpp
)

add_subdirectory(nested/)
add_subdirectory(offscreen/)
