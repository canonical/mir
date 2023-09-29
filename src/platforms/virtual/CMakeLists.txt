add_library(mirplatformvirtual-graphics STATIC
    platform.cpp                platform.h
    display.cpp                 display.h
    display_configuration.cpp   display_configuration.h
)

target_include_directories(mirplatformvirtual-graphics
    PUBLIC
    ${server_common_include_dirs}
)

target_link_libraries(mirplatformvirtual-graphics
    PUBLIC
    mirplatform
    server_platform_common
)

add_library(mirplatformvirtual MODULE
    platform_symbols.cpp
)

target_link_libraries(mirplatformvirtual
    PRIVATE
    mirplatformvirtual-graphics
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

set_target_properties(
    mirplatformvirtual PROPERTIES
    OUTPUT_NAME graphics-virtual
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
    PREFIX ""
    SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformvirtual LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
