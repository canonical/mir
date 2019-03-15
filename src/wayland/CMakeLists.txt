set(MIRWAYLAND_ABI 0)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirwayland")

add_subdirectory(generator/)

get_filename_component(
        PROTOCOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protocol/
        ABSOLUTE
        BASE_DIR ${PROJECT_SOURCE_DIR}
)

get_filename_component(
        GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated/
        ABSOLUTE
        BASE_DIR ${PROJECT_SOURCE_DIR}
)

set(GENERATED_FILES "")

macro(GENERATE_PROTOCOL NAME_PREFIX PROTOCOL_NAME)
    set(PROTOCOL_PATH "${PROTOCOL_DIR}/${PROTOCOL_NAME}.xml")
    set(OUTPUT_PATH_HEADER "${GENERATED_DIR}/${PROTOCOL_NAME}_wrapper.h")
    set(OUTPUT_PATH_SRC "${GENERATED_DIR}/${PROTOCOL_NAME}_wrapper.cpp")
    add_custom_command(OUTPUT "${OUTPUT_PATH_HEADER}"
            VERBATIM
            COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator ${NAME_PREFIX} ${PROTOCOL_PATH} header > ${OUTPUT_PATH_HEADER}"
            DEPENDS "${PROTOCOL_PATH}"
            DEPENDS wrapper-generator
            )
    add_custom_command(OUTPUT "${OUTPUT_PATH_SRC}"
            VERBATIM
            COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator ${NAME_PREFIX} ${PROTOCOL_PATH} source > ${OUTPUT_PATH_SRC}"
            DEPENDS "${PROTOCOL_PATH}"
            DEPENDS wrapper-generator
            )
    set(GENERATED_FILES ${GENERATED_FILES} "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}")
endmacro()

# when adding a protocol, don't forget to add the generated .c file to CMake
GENERATE_PROTOCOL("wl_" "wayland")
GENERATE_PROTOCOL("z" "xdg-shell-unstable-v6")
GENERATE_PROTOCOL("_" "xdg-shell") # empty prefix is not allowed, but '_' won't match anything, so it is ignored
GENERATE_PROTOCOL("zwlr_" "wlr-layer-shell-unstable-v1")

add_custom_target(refresh-wayland-wrapper
    DEPENDS ${GENERATED_FILES}
    DEPENDS wrapper-generator
    SOURCES ${GENERATED_FILES}
)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

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

