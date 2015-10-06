include_directories(${GLESv2_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})

add_library(
  mirgraphics OBJECT

  default_configuration.cpp
  default_display_configuration_policy.cpp
  gl_extensions_base.cpp
  surfaceless_egl_context.cpp
  software_cursor.cpp
)

add_subdirectory(nested/)
add_subdirectory(offscreen/)
