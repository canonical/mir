pkg_check_modules(LIBDISPLAY_INFO REQUIRED IMPORTED_TARGET libdisplay-info)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirplatform")

include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_compile_definitions(MIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_compile_definitions(MIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

set(MIR_PLATFORM_REFERENCES
  PkgConfig::EGL
  PkgConfig::GLESv2
  mirwayland
  mirplatformgraphicscommon
  miroptions
  mirudev
)

add_subdirectory(graphics/)
add_subdirectory(options)
add_subdirectory(udev)

set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)

add_library(mirplatform SHARED
  ${PROJECT_SOURCE_DIR}/include/platform/mir/input/input_sink.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/console_services.h
)

target_include_directories(mirplatform
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
)

target_link_libraries(mirplatform
  PRIVATE
    mircommon
    ${MIR_PLATFORM_REFERENCES}
  PUBLIC
    mirwayland
    mirplatformgraphicscommon
    miroptions
    mirudev
    PkgConfig::EPOXY
)

set_target_properties(
  mirplatform

  PROPERTIES
  SOVERSION ${MIRPLATFORM_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirplatform.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
