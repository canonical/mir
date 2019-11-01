find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL REQUIRED wayland-egl)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="wayland")

add_library(mirplatformwayland-graphics STATIC
    platform.cpp                platform.h
    display.cpp                 display.h
    buffer_allocator.cpp        buffer_allocator.h
        displayclient.cpp displayclient.h
    wayland_display.cpp         wayland_display.h
    cursor.cpp                  cursor.h
)

target_include_directories(mirplatformwayland-graphics
PUBLIC
    ${server_common_include_dirs}
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/client
    ${GBM_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${EPOXY_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${WAYLAND_EGL_INCLUDE_DIRS}
)

target_link_libraries(mirplatformwayland-graphics
PUBLIC
    mirplatform
    server_platform_common
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
    ${EPOXY_LIBRARIES}
    ${WAYLAND_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${DRM_LIBRARIES}
    ${GBM_LIBRARIES}
    ${XKBCOMMON_LIBRARIES}
)

add_library(mirplatformwayland-input STATIC
  input_platform.cpp    input_platform.h
  input_device.cpp      input_device.h
)

target_include_directories(mirplatformwayland-input
PUBLIC
    ${server_common_include_dirs}
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/client
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
