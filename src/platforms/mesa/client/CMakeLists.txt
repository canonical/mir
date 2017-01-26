include_directories(${client_common_include_dirs})

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformmesaobjects OBJECT
  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
  mesa_native_display_container.cpp
  native_surface.cpp
)

add_library(mirclientplatformmesa MODULE
  $<TARGET_OBJECTS:mirclientplatformmesaobjects>
)

set_target_properties(
  mirclientplatformmesa PROPERTIES
  OUTPUT_NAME mesa
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/client-modules
  PREFIX ""
  SUFFIX ".so.${MIR_CLIENT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

target_link_libraries(mirclientplatformmesa
  mirclient
  mirsharedmesa-static
  client_platform_common
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

install(TARGETS mirclientplatformmesa LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
