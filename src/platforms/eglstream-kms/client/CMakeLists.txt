include_directories(${client_common_include_dirs})

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformeglstreamobjects OBJECT
  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
)

mir_add_library_with_symbols(
  mirclientplatformeglstream MODULE

  ${symbol_map}
  $<TARGET_OBJECTS:mirclientplatformeglstreamobjects>
)

set_target_properties(
  mirclientplatformeglstream PROPERTIES
  OUTPUT_NAME eglstream
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/client-modules
  PREFIX ""
  SUFFIX ".so.${MIR_CLIENT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

target_link_libraries(mirclientplatformeglstream
  mirclient
  client_platform_common
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

install(TARGETS mirclientplatformeglstream LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
