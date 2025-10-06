set(MIRWAYLAND_ABI 5)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirwayland")

add_subdirectory(generator/)

set(STANDARD_SOURCES
  lifetime_tracker.cpp
  resource.cpp
  global.cpp
  protocol_error.cpp
  client.cpp
)

add_library(mirwayland SHARED
    ${STANDARD_SOURCES}
)

mir_generate_protocol_wrapper(mirwayland "wl_" wayland.xml)
mir_generate_protocol_wrapper(mirwayland "z" xdg-shell-unstable-v6.xml)
mir_generate_protocol_wrapper(mirwayland "" xdg-shell.xml)
mir_generate_protocol_wrapper(mirwayland "z" xdg-output-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwlr_" wlr-layer-shell-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwlr_" wlr-foreign-toplevel-management-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" pointer-constraints-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" relative-pointer-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" virtual-keyboard-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" text-input-unstable-v3.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" text-input-unstable-v2.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" text-input-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" input-method-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" input-method-unstable-v2.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" idle-inhibit-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_" primary-selection-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "z" wlr-screencopy-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwlr_" wlr-virtual-pointer-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-session-lock-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zmir_" mir-shell-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "z" xdg-decoration-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "wp_" viewporter.xml)
mir_generate_protocol_wrapper(mirwayland "wp_" fractional-scale-v1.xml)
mir_generate_protocol_wrapper(mirwayland "z" xdg-activation-v1.xml)
mir_generate_protocol_wrapper(mirwayland "wp_" linux-drm-syncobj-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-data-control-v1.xml)
# We can't strip the "ext_" prefix here, because it creates a conflict
# with wlr-foreign-toplevel-management-unstable-v1.
mir_generate_protocol_wrapper(mirwayland "" ext-foreign-toplevel-list-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-image-capture-source-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-image-copy-capture-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-input-trigger-registration-v1.xml)
mir_generate_protocol_wrapper(mirwayland "ext_" ext-input-trigger-activation-v1.xml)

target_link_libraries(mirwayland
  PUBLIC
    mircore
    PkgConfig::WAYLAND_SERVER
  PRIVATE
    mircommon
)

target_include_directories(mirwayland
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/wayland
    ${PROJECT_SOURCE_DIR}/src/include/wayland
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(mirwayland
    PROPERTIES
    SOVERSION ${MIRWAYLAND_ABI}
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mirwayland.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mirwayland.pc
    @ONLY
)

install(TARGETS     mirwayland                           LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/mirwayland.pc    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/wayland/mir      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirwayland")
