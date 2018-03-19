add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirplatform")

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_definitions(-DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_definitions(-DMIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

set(MIR_PLATFORM_OBJECTS
  $<TARGET_OBJECTS:mirplatformgraphicscommon>
  $<TARGET_OBJECTS:miroptions>
  $<TARGET_OBJECTS:mirudev>
)

set(MIR_PLATFORM_REFERENCES
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
)

add_subdirectory(graphics/)
add_subdirectory(options)
add_subdirectory(udev)

set(MIR_PLATFORM_OBJECTS ${MIR_PLATFORM_OBJECTS} PARENT_SCOPE)
set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)

add_library(mirplatform SHARED
  ${MIR_PLATFORM_OBJECTS}

  ${PROJECT_SOURCE_DIR}/include/platform/mir/input/input_sink.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/console_services.h
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
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

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
