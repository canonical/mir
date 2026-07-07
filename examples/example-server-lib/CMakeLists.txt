find_package(PIL REQUIRED)
pkg_check_modules(FREETYPE freetype2 REQUIRED IMPORTED_TARGET)

set(XDG_SHELL_H "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h")
set(XDG_SHELL_C "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c")
set(XDG_SHELL_X "${PROJECT_SOURCE_DIR}/wayland-protocols/xdg-shell.xml")

add_custom_command(
    OUTPUT "${XDG_SHELL_H}" "${XDG_SHELL_C}"
    VERBATIM
    COMMAND "sh" "-c" "wayland-scanner client-header ${XDG_SHELL_X} ${XDG_SHELL_H}"
    COMMAND "sh" "-c" "wayland-scanner private-code  ${XDG_SHELL_X} ${XDG_SHELL_C}"
    DEPENDS "${XDG_SHELL_X}"
)

add_library(example-shell-lib STATIC
    tiling_window_manager.cpp   tiling_window_manager.h
    floating_window_manager.cpp floating_window_manager.h
    kiosk_window_manager_policy.cpp kiosk_window_manager_policy.h
    decoration_provider.cpp     decoration_provider.h
    wallpaper_config.cpp        wallpaper_config.h
        splash_session.h
    sw_splash.cpp               sw_splash.h
    wayland_app.cpp             wayland_app.h
    wayland_surface.cpp         wayland_surface.h
    wayland_shm.cpp             wayland_shm.h
    ${XDG_SHELL_C}              ${XDG_SHELL_H}
)

target_include_directories(example-shell-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(example-shell-lib PUBLIC miral PRIVATE PkgConfig::WAYLAND_CLIENT PkgConfig::FREETYPE)
