find_package(PIL REQUIRED)
pkg_check_modules(FREETYPE freetype2 REQUIRED IMPORTED_TARGET)

add_library(example-shell-lib STATIC
    tiling_window_manager.cpp   tiling_window_manager.h
    floating_window_manager.cpp floating_window_manager.h
    decoration_provider.cpp     decoration_provider.h
    wallpaper_config.cpp        wallpaper_config.h
        splash_session.h
    sw_splash.cpp               sw_splash.h
    wayland_app.cpp             wayland_app.h
    wayland_surface.cpp         wayland_surface.h
    wayland_shm.cpp             wayland_shm.h
    reloading_config_file.cpp   reloading_config_file.h
)

target_include_directories(example-shell-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(example-shell-lib PUBLIC miral PRIVATE PkgConfig::WAYLAND_CLIENT PkgConfig::FREETYPE)
