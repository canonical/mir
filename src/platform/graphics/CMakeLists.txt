# Workaround for bug 1208774 (spurious link errors on 32bit g++)
string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
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

  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
