configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
    )
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(
  mirplatformrenderersoftwareblitter MODULE

  blitter_provider.cpp
  blitter_provider.h
  platform_symbols.cpp
  rendering_platform.cpp
  rendering_platform.h
)

target_link_libraries(
  mirplatformrenderersoftwareblitter

  PRIVATE
    mirplatform
    mirplatformrenderereglgenericobjects
    PkgConfig::WAYLAND_SERVER
)

set_target_properties(
  mirplatformrenderersoftwareblitter
  PROPERTIES
    OUTPUT_NAME renderer-software-blitter
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
    PREFIX ""
    SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformrenderersoftwareblitter LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
