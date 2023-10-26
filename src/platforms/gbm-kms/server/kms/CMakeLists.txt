include_directories(
    ${server_common_include_dirs}
    ..
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/../
)

# gbm-kms.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_compile_definitions(
  __GBM__
  MIR_LOG_COMPONENT_FALLBACK="gbm-kms"
)

add_library(
  mirplatformgraphicsgbmkmsobjects OBJECT

  bypass.cpp
  cursor.cpp
  display.cpp
  display_buffer.cpp
  page_flipper.h
  kms_page_flipper.cpp
  platform.cpp
  kms_display_configuration.h
  real_kms_display_configuration.cpp
  kms_output.h
  real_kms_output.h
  real_kms_output.cpp
  kms_output_container.h
  real_kms_output_container.cpp
  egl_helper.h
  egl_helper.cpp
  quirks.cpp
  quirks.h
)

target_link_libraries(
  mirplatformgraphicsgbmkmsobjects

  PRIVATE
    mirsharedgbmservercommon-static
    ${GBM_LDFLAGS} ${GBM_LIBRARIES}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
    ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
    )
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformgraphicsgbmkms MODULE
  platform_symbols.cpp
)

target_link_libraries(
  mirplatformgraphicsgbmkms

  PRIVATE
    mirplatform
    mirplatformgraphicsgbmkmsobjects
    mirsharedgbmservercommon-static
    Boost::program_options
    Boost::iostreams
    PkgConfig::DRM
    PkgConfig::GBM
    PkgConfig::EGL
    PkgConfig::GLESv2
    PkgConfig::WAYLAND_SERVER
)

set_target_properties(
  mirplatformgraphicsgbmkms PROPERTIES
  OUTPUT_NAME graphics-gbm-kms
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformgraphicsgbmkms LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
