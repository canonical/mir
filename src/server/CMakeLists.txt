pkg_check_modules(FREETYPE freetype2 REQUIRED IMPORTED_TARGET) # For rendering decorated window titles

add_compile_definitions(
  MIR_LOG_COMPONENT_FALLBACK="mirserver"
  MIR_VERSION="${MIR_VERSION}"
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/renderer
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/wayland
)
add_compile_definitions(MIR_SERVER_INPUT_PLATFORM_VERSION="${MIR_SERVER_INPUT_PLATFORM_VERSION}")

add_compile_definitions(MIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_compile_definitions(MIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

add_subdirectory(compositor/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(report/)
add_subdirectory(scene/)
add_subdirectory(frontend_wayland/)
add_subdirectory(frontend_xwayland/)
add_subdirectory(shell/)
add_subdirectory(console/)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
  @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mirserver-internal.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mirserver-internal.pc
    @ONLY
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
  shm_backing.cpp
  shm_backing.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/time/alarm_factory.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/time/alarm.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/observer_registrar.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/observer_multiplexer.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/glib_main_loop.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/glib_main_loop_sources.h
)

target_link_libraries(mirserverobjects
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::GLIB
    PkgConfig::GIO
)

set_property(
    SOURCE glib_main_loop.cpp glib_main_loop_sources.cpp default_server_configuration.cpp
    PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

set(MIR_SERVER_REFERENCES
  mirserverobjects
  mirinput
  mirscene
  mircompositor
  mirgraphics
  mirfrontend-wayland
  mirfrontend-xwayland
  mirshell
  mirshelldecoration
  mirlttng
  mirreport
  mirlogging
  mirnullreport
  mirconsole

  mirrenderergl
  mirgl

  PkgConfig::DRM
  PkgConfig::EGL
  PkgConfig::GLESv2
  PkgConfig::UDEV
  PkgConfig::GLIB
  PkgConfig::GIO
  PkgConfig::UUID
  PkgConfig::LTTNG_UST
)

set(MIR_SERVER_REFERENCES ${MIR_SERVER_REFERENCES} PARENT_SCOPE)

add_library(mirserver SHARED)

target_link_libraries(mirserver
  PUBLIC


    mirplatform
    mircommon
    mirwayland

    server_platform_common
    Boost::iostreams
    PkgConfig::DRM
    PkgConfig::EGL
    PkgConfig::GLESv2
    PkgConfig::UDEV
    PkgConfig::GLIB
    PkgConfig::GIO
    PkgConfig::UUID
    PkgConfig::XCB
    PkgConfig::XCB_COMPOSITE
    PkgConfig::XCB_XFIXES
    PkgConfig::XCB_RANDR
    PkgConfig::XCB_RENDER
    PkgConfig::XCB_RES
    PkgConfig::X11_XCURSOR
    PkgConfig::LTTNG_UST
    PkgConfig::FREETYPE
    atomic

  PRIVATE
    ${MIR_SERVER_REFERENCES}
)

if (MIR_USE_APPARMOR)
  target_link_libraries(mirserver PUBLIC PkgConfig::APPARMOR)
endif()

set(MIRSERVER_INCLUDE_DIRS ${UUID_INCLUDE_DIRS} PARENT_SCOPE)

install(TARGETS mirserver
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/include/platform/mir DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirplatform"
)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/include/server/mir DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirserver"
)
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/src/include/server/mir DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirserver-internal"
)

set(MIRSERVER_ABI 65) # Be sure to increment MIR_VERSION_MINOR at the same time
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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirserver-internal.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

add_custom_target(
    generate-mirserver-symbols-map
    COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name mir_server
        --version ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --internal-headers-directory="${PROJECT_SOURCE_DIR}/src/include/server"
        --include-dirs="$<JOIN:$<TARGET_PROPERTY:mirserver,INCLUDE_DIRECTORIES>,:>"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_custom_target(
  check-mirserver-symbols-map
  COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
      --library-name mir_server
      --version ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}
      --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
      --internal-headers-directory="${PROJECT_SOURCE_DIR}/src/include/server"
      --include-dirs "$<JOIN:$<TARGET_PROPERTY:mirserver,INCLUDE_DIRECTORIES>,:>"
      --diff
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
