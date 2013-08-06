include_directories(${GLESv2_INCLUDE_DIRS})

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

  ${MIR_COMMON_PLATFORM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
