add_subdirectory(graphics/)
add_subdirectory(input/)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformvirtual MODULE
  $<TARGET_OBJECTS:mirplatformgraphicsvirtualobjects>
  $<TARGET_OBJECTS:mirplatformgraphicsvirtualobjects-symbols>
  $<TARGET_OBJECTS:mirplatforminputvirtualobjects>
  $<TARGET_OBJECTS:mirplatforminputvirtualobjects-symbols>
)


target_link_libraries(
  mirplatformvirtual
  PRIVATE
  mirplatform
  server_platform_common
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
