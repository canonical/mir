set(MIRWAYLAND_ABI 3)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirwayland")

add_subdirectory(generator/)

set(STANDARD_SOURCES
  wayland_base.cpp
)

add_library(mirwayland SHARED
    ${STANDARD_SOURCES}
)

mir_generate_protocol_wrapper(mirwayland "wl_"   protocol/wayland.xml)
mir_generate_protocol_wrapper(mirwayland "z"     protocol/xdg-shell-unstable-v6.xml)
mir_generate_protocol_wrapper(mirwayland ""      protocol/xdg-shell.xml)
mir_generate_protocol_wrapper(mirwayland "z"     protocol/xdg-output-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwlr_" protocol/wlr-layer-shell-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwlr_" protocol/wlr-foreign-toplevel-management-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/pointer-constraints-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/relative-pointer-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/virtual-keyboard-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/text-input-unstable-v3.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/text-input-unstable-v2.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/input-method-unstable-v2.xml)
mir_generate_protocol_wrapper(mirwayland "zwp_"  protocol/idle-inhibit-unstable-v1.xml)
mir_generate_protocol_wrapper(mirwayland "z"     protocol/wlr-screencopy-unstable-v1.xml)

target_link_libraries(mirwayland
  PUBLIC
    mircore
    ${WAYLAND_SERVER_LIBRARIES}
  PRIVATE
    mircommon
)

target_include_directories(mirwayland
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/wayland
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

install(TARGETS     mirwayland                           LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/mirwayland.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/wayland/mir      DESTINATION "include/mirwayland")

