add_subdirectory(graphics/)
add_subdirectory(input/)

add_library(
  mirplatformservermesax11sharedresources OBJECT
  X11_resources.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformservermesax11 MODULE
  $<TARGET_OBJECTS:mirplatformservermesax11sharedresources>
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects-symbols>
  $<TARGET_OBJECTS:mirplatforminputmesax11objects>
  $<TARGET_OBJECTS:mirplatforminputmesax11objects-symbols>
)


target_link_libraries(
  mirplatformservermesax11
  PRIVATE
  mirplatform
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
  X11
  Xfixes
  mirsharedmesaservercommon-static
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
)

set_target_properties(
  mirplatformservermesax11 PROPERTIES
  OUTPUT_NAME server-mesa-x11
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformservermesax11 LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
