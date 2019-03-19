set(MIRWAYLAND_ABI 0)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirwayland")

add_subdirectory(generator/)
add_subdirectory(generated/)

add_library(mirwayland SHARED
    ${GENERATED_FILES}
)

target_link_libraries(mirwayland
  LINK_PUBLIC
    mircore
    ${WAYLAND_SERVER_LIBRARIES}
  LINK_PRIVATE
    mircommon
)

target_include_directories(mirwayland
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

set_target_properties(mirwayland
    PROPERTIES
    SOVERSION ${MIRWAYLAND_ABI}
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mirwayland.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mirwayland.pc
    @ONLY
)

install(TARGETS     mirwayland                           LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/mirwayland.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")

