include_directories(
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

add_library(
  mirclientplatform SHARED

  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
  mesa_native_display_container.cpp
  native_surface.cpp
)

target_link_libraries(
  mirclientplatform
  mirclient

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

install(TARGETS mirclientplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

