pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})

add_executable(wrapper-generator

  wrapper_generator.cpp
)

target_link_libraries(wrapper-generator

  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)

get_filename_component(
  CORE_GENERATED_HEADER src/server/frontend_wayland/core_generated_interfaces.h
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

get_filename_component(
  XDG_SHELL_GENERATED_HEADER src/server/frontend_wayland/xdg_shell_generated_interfaces.h
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

get_filename_component(
  XDG_SHELL_HEADER src/server/frontend_wayland/xdg-shell-unstable-v6.h
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

get_filename_component(
  XDG_SHELL_CODE src/server/frontend_wayland/xdg-shell-unstable-v6.c
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

add_custom_target(refresh-wayland-wrapper
  COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator wl_ wayland-server.h /usr/share/wayland/wayland.xml >${CORE_GENERATED_HEADER}"
  COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator z xdg-shell-unstable-v6.h /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml >${XDG_SHELL_GENERATED_HEADER}"
  COMMAND "wayland-scanner" "--include-core-only" "server-header" "/usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml" "${XDG_SHELL_HEADER}"
  COMMAND "wayland-scanner" "--include-core-only" "code" "/usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml" "${XDG_SHELL_CODE}"
  VERBATIM
  DEPENDS wrapper-generator
  DEPENDS /usr/share/wayland/wayland.xml
  DEPENDS /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml
  SOURCES ${CORE_GENERATED_HEADER} ${XDG_SHELL_GENERATED_HEADER}
)

