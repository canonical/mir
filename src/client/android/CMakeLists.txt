include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})

add_library(
  mirclientplatform SHARED

  android_client_buffer.cpp
  android_client_buffer_factory.cpp
  android_registrar.cpp
  android_client_platform.cpp
  client_platform_factory.cpp
  client_surface_interpreter.cpp
  android_native_display_container.cpp
)

target_link_libraries(
  mirclientplatform
  mirclient

  ${LIBHARDWARE_LIBRARIES}
)

install(TARGETS mirclientplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
