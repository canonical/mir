configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
    )
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(
  mirplatformrenderereglgeneric MODULE

  platform_symbols.cpp
)

target_link_libraries(
  mirplatformrenderereglgeneric

  PRIVATE
    mirplatformrenderereglcommonobjects
    mirplatform
    PkgConfig::EGL
    PkgConfig::GLESv2
    PkgConfig::WAYLAND_SERVER
)

target_include_directories(
  mirplatformrenderereglgeneric

  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/platforms
)

set_target_properties(
  mirplatformrenderereglgeneric
  PROPERTIES
    OUTPUT_NAME renderer-egl-generic
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
    PREFIX ""
    SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformrenderereglgeneric LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
