pkg_check_modules(UUID REQUIRED uuid)

add_definitions(
  -DMIR_LOG_COMPONENT_FALLBACK="mirserver"
  -DMIR_VERSION="${MIR_VERSION}"
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/renderer
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
)
add_definitions(-DMIR_SERVER_INPUT_PLATFORM_VERSION="${MIR_SERVER_INPUT_PLATFORM_VERSION}")

add_definitions(-DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_definitions(-DMIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

add_subdirectory(compositor/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(report/)
add_subdirectory(scene/)
add_subdirectory(frontend/)
add_subdirectory(frontend_wayland/)
add_subdirectory(shell/)
add_subdirectory(thread/)
add_subdirectory(console/)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirserver")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${PROJECT_SOURCE_DIR}/include/server/mir/version.h
)

add_library(mirserverobjects OBJECT
  run_mir.cpp
  report_exception.cpp
  terminate_with_current_exception.cpp
  display_server.cpp
  default_server_configuration.cpp
  glib_main_loop.cpp
  glib_main_loop_sources.cpp
  default_emergency_cleanup.cpp
  server.cpp
  lockable_callback_wrapper.cpp
  basic_callback.cpp
  ${PROJECT_SOURCE_DIR}/include/server/mir/time/alarm_factory.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/time/alarm.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/observer_registrar.h
  ${PROJECT_SOURCE_DIR}/include/server/mir/executor.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/observer_multiplexer.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/glib_main_loop.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/glib_main_loop_sources.h
)

set(MIR_SERVER_OBJECTS
  $<TARGET_OBJECTS:mirserverobjects>
  $<TARGET_OBJECTS:mirinput>
  $<TARGET_OBJECTS:mirscene>
  $<TARGET_OBJECTS:mircompositor>
  $<TARGET_OBJECTS:mirgraphics>
  $<TARGET_OBJECTS:mirfrontend>
  $<TARGET_OBJECTS:mirfrontend-wayland>
  $<TARGET_OBJECTS:mirfrontend-waylandgenerated>
  $<TARGET_OBJECTS:mirshell>
  $<TARGET_OBJECTS:mirlttng>
  $<TARGET_OBJECTS:mirreport>
  $<TARGET_OBJECTS:mirlogging>
  $<TARGET_OBJECTS:mirnullreport>
  $<TARGET_OBJECTS:mirnestedgraphics>
  $<TARGET_OBJECTS:miroffscreengraphics>
  $<TARGET_OBJECTS:mirthread>
  $<TARGET_OBJECTS:mirconsole>

  $<TARGET_OBJECTS:mirrenderergl>
  $<TARGET_OBJECTS:mirgl>
)

set(MIR_SERVER_REFERENCES
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
  ${UDEV_LDFLAGS} ${UDEV_LIBRARIES}
  ${GLIB_LDFLAGS} ${GLIB_LIBRARIES}
  ${GIO_LDFLAGS} ${GIO_LIBRARIES}
  ${UUID_LDFLAGS} ${UUID_LIBRARIES}
)

set(MIR_SERVER_OBJECTS ${MIR_SERVER_OBJECTS} PARENT_SCOPE)
set(MIR_SERVER_REFERENCES ${MIR_SERVER_REFERENCES} PARENT_SCOPE)

add_library(mirserver SHARED
  ${MIR_SERVER_OBJECTS}
)

target_link_libraries(mirserver LINK_PUBLIC

  mirclient
  mirplatform
  mircommon
  mirprotobuf
  mircookie

  server_platform_common
  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
  ${UDEV_LDFLAGS} ${UDEV_LIBRARIES}
  ${GLIB_LDFLAGS} ${GLIB_LIBRARIES}
  ${GIO_LDFLAGS} ${GIO_LIBRARIES}
  ${UUID_LDFLAGS} ${UUID_LIBRARIES}
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
  atomic
)

target_include_directories(mirshell
  PUBLIC
    ${UUID_INCLUDE_DIRS}
)

set(MIRSERVER_INCLUDE_DIRS ${UUID_INCLUDE_DIRS} PARENT_SCOPE)

install(TARGETS mirserver
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/include/platform/mir DESTINATION "include/mirplatform"
)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/include/server/mir DESTINATION "include/mirserver"
)

set(MIRSERVER_ABI 47) # Be sure to increment MIR_VERSION_MINOR at the same time
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(
  mirserver

  PROPERTIES
  SOVERSION ${MIRSERVER_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
