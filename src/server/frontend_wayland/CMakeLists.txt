# When cross compiling, the generator needs to be compiled for the host system, not the target one

set(mir_wayland_scanner_cmd "${CMAKE_BINARY_DIR}/bin/wayland-scanner${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_command(
  OUTPUT ${mir_wayland_scanner_cmd}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generator/
  COMMAND "sh" "-c" "CXX=/usr/bin/c++ ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=\"Release\" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH=\"${CMAKE_BINARY_DIR}/bin\"  -B\"bin\" -H\"${CMAKE_CURRENT_SOURCE_DIR}/generator/\""
  COMMAND "sh" "-c" "CXX=/usr/bin/c++ ${CMAKE_COMMAND} --build bin --config Release"
)

add_custom_target(
  mir-wayland-scanner
  DEPENDS ${mir_wayland_scanner_cmd}
)

add_subdirectory(generated/)
add_subdirectory(protocol/)

include_directories(../frontend_xwayland)

set(
  WAYLAND_SOURCES

  wayland_default_configuration.cpp
  wayland_connector.cpp         wayland_connector.h
  wlshmbuffer.cpp               wlshmbuffer.h
  wayland_executor.cpp          wayland_executor.h
  null_event_sink.cpp           null_event_sink.h
  wl_surface_event_sink.cpp     wl_surface_event_sink.h
  data_device.cpp               data_device.h
  output_manager.cpp            output_manager.h
  wl_subcompositor.cpp          wl_subcompositor.h
                                wl_surface_role.h
  window_wl_surface_role.cpp    window_wl_surface_role.h
  wl_surface.cpp                wl_surface.h
  wl_seat.cpp                   wl_seat.h
  wl_keyboard.cpp               wl_keyboard.h
  wl_pointer.cpp                wl_pointer.h
  wl_touch.cpp                  wl_touch.h
  xdg_shell_v6.cpp              xdg_shell_v6.h
  xdg_shell_stable.cpp          xdg_shell_stable.h
  deleted_for_resource.cpp      deleted_for_resource.h
  wl_region.cpp                 wl_region.h)

add_library(
  mirfrontend-wayland OBJECT
  ${WAYLAND_SOURCES}
)

add_dependencies(
  mirfrontend-wayland
  mirfrontend-waylandgenerated
)
