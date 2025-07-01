include_directories(
    ${server_common_include_dirs}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_compile_definitions(MIR_LOG_COMPONENT="mir:virtual")

add_library(mirplatformgraphicsvirtualobjects OBJECT
    graphics.cpp
    platform.cpp
    platform.h
    display.h
    display.cpp
    display_configuration.h
    display_configuration.cpp
)

target_link_libraries(mirplatformgraphicsvirtualobjects
    PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(mirplatformvirtual MODULE
)

target_link_libraries(
    mirplatformvirtual

    PRIVATE
    mirplatform
    server_platform_common
    mirplatformgraphicsvirtualobjects
)

set_target_properties(
    mirplatformvirtual PROPERTIES
    OUTPUT_NAME server-virtual
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
    PREFIX ""
    SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformvirtual LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
