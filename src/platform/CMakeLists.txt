include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(MIRPLATFORM_ABI 4)

set(PLATFORM_DRIVER platform${MIRPLATFORM_ABI}driver)
set(MIR_PLATFORM_DRIVER mir${PLATFORM_DRIVER})
set(MIR_PLATFORM_DRIVER_BINARY lib${MIR_PLATFORM_DRIVER}.so
    CACHE STRING "File name of Mir server platform drivers")
add_definitions(-DMIR_PLATFORM_DRIVER_BINARY="${MIR_PLATFORM_DRIVER_BINARY}")

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirplatformobjects OBJECT
  shared_library_loader.cpp
)

set(MIR_PLATFORM_OBJECTS
  $<TARGET_OBJECTS:mirplatformobjects>
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

set_target_properties(mirplatform
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
