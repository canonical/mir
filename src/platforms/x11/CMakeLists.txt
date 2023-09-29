add_subdirectory(graphics/)
add_subdirectory(input/)

add_library(
  mirplatformserverx11sharedresources OBJECT
  x11_resources.cpp
)

target_link_libraries(mirplatformserverx11sharedresources
  PUBLIC
    mircommon
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformserverx11 MODULE
  $<TARGET_OBJECTS:mirplatformserverx11sharedresources>
  $<TARGET_OBJECTS:mirplatformgraphicsx11objects>
  $<TARGET_OBJECTS:mirplatformgraphicsx11objects-symbols>
  $<TARGET_OBJECTS:mirplatforminputx11objects>
  $<TARGET_OBJECTS:mirplatforminputx11objects-symbols>
)


target_link_libraries(
  mirplatformserverx11
  PRIVATE
  mirplatform
  server_platform_common
  PkgConfig::EGL
  PkgConfig::GLESv2
  PkgConfig::XCB
  PkgConfig::XCB_COMPOSITE
  PkgConfig::XCB_XFIXES
  PkgConfig::XCB_RENDER
  xkbcommon
  xcb-xkb
  xkbcommon-x11
  X11
  X11-xcb
  Boost::program_options
)

set_target_properties(
  mirplatformserverx11 PROPERTIES
  OUTPUT_NAME server-x11
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformserverx11 LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
