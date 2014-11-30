add_definitions(-DMIR_LOGGING_COMPONENT_FALLBACK="mirplatform")

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(MIRPLATFORM_ABI 4)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

# This needs to change whenever the ABI between the platform
# modules and the server changes in an incompatible way.
# This ABI is much smaller than the full libmirplatform ABI.
#
# TODO: Add an extra driver-ABI check target.
set(SERVER_PLATFORM_VERSION "MIR_GRAPHICS_PLATFORM_1")
set(SERVER_PLATFORM_VERSION ${SERVER_PLATFORM_VERSION} PARENT_SCOPE)

set(MIR_SERVER_PLATFORM_PATH
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/mir/server-platform
)
set(MIR_SERVER_PLATFORM_PATH
  ${MIR_SERVER_PLATFORM_PATH}
  PARENT_SCOPE
)

add_definitions(-DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_definitions(-DSERVER_PLATFORM_VERSION="${SERVER_PLATFORM_VERSION}")

set(MIR_PLATFORM_OBJECTS
  $<TARGET_OBJECTS:mirplatformgraphicscommon>
  $<TARGET_OBJECTS:miroptions>
  $<TARGET_OBJECTS:mirfatal>
)

set(MIR_PLATFORM_REFERENCES
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

set(MIR_PLATFORM_OBJECTS ${MIR_PLATFORM_OBJECTS} PARENT_SCOPE)
set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)

add_library(mirplatform SHARED
  ${MIR_PLATFORM_OBJECTS}
)

target_link_libraries(mirplatform

  mircommon
  ${MIR_PLATFORM_REFERENCES}
)

set_target_properties(
  mirplatform

  PROPERTIES
  SOVERSION ${MIRPLATFORM_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(graphics/)
add_subdirectory(options)
add_subdirectory(fatal)

set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirplatform")
set(COMMON_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirplatform.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
