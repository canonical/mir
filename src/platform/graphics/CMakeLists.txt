set(
  GRAPHICS_SOURCES

  egl_extensions.cpp
  egl_resources.cpp
  display_configuration.cpp
  null_display_report.cpp
  default_display_configuration_policy.cpp
  buffer_basic.cpp
)

add_library(
  mirplatform SHARED

  ${GRAPHICS_SOURCES}
)

target_link_libraries(
  mirplatform
  mirsharedgeometry

  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
