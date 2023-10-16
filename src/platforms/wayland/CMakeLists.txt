pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_EGL REQUIRED IMPORTED_TARGET wayland-egl)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED IMPORTED_TARGET)

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="wayland")

add_library(mirplatformwayland-graphics STATIC
    platform.cpp                platform.h
    display.cpp                 display.h
    displayclient.cpp           displayclient.h
    wayland_display.cpp         wayland_display.h
    cursor.cpp                  cursor.h
    wl_egl_display_target.cpp   wl_egl_display_target.h
)

target_include_directories(mirplatformwayland-graphics
PUBLIC
    ${server_common_include_dirs}
)

target_link_libraries(mirplatformwayland-graphics
PUBLIC
    mirplatform
    server_platform_common
    Boost::program_options
    PkgConfig::EGL
    PkgConfig::GLESv2
    PkgConfig::EPOXY
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::WAYLAND_EGL
    PkgConfig::DRM
    PkgConfig::GBM
    PkgConfig::XKBCOMMON
)

add_library(mirplatformwayland-input STATIC
  input_platform.cpp    input_platform.h
  input_device.cpp      input_device.h
)

target_link_libraries(mirplatformwayland-input
  PUBLIC
    mirplatform
    mircommon
    mircore
)

target_include_directories(mirplatformwayland-input
PUBLIC
    ${server_common_include_dirs}
)

add_library(mirplatformwayland MODULE
    input.cpp
    platform_symbols.cpp
)

target_link_libraries(mirplatformwayland
    PRIVATE
        mirplatformwayland-graphics
        mirplatformwayland-input
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_subdirectory(protocol)

set_target_properties(
    mirplatformwayland PROPERTIES
    OUTPUT_NAME graphics-wayland
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
    PREFIX ""
    SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformwayland LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
