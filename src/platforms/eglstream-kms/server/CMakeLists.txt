include_directories(
  ${server_common_include_dirs}
  ${DRM_INCLUDE_DIRS}
  ${GBM_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${EPOXY_INCLUDE_DIRS}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
  ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformgraphicseglstreamkmsobjects OBJECT 
  platform_symbols.cpp
  platform.cpp
  buffer_allocator.cpp
  buffer_allocator.h
  display.cpp
  display.h
  kms_display_configuration.h
  kms_display_configuration.cpp
  egl_output.h
  egl_output.cpp
  software_buffer.cpp
  utils.cpp
  utils.h
)

add_library(mirplatformgraphicseglstreamkms MODULE
  $<TARGET_OBJECTS:mirplatformgraphicseglstreamkmsobjects>
)

target_link_libraries(
  mirplatformgraphicseglstreamkms

  PRIVATE
  mirplatform
  server_platform_common
  kms_utils
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${EPOXY_LDFLAGS} ${EPOXY_LIBRARIES}
)

set_target_properties(
  mirplatformgraphicseglstreamkms PROPERTIES
  OUTPUT_NAME graphics-eglstream-kms
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformgraphicseglstreamkms LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
