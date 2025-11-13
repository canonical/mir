find_package(PkgConfig REQUIRED)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

# where to place generated headers/sources for this example
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

# list the protocol XML files (relative to project source)
set(PROTOCOL_DIR ${CMAKE_SOURCE_DIR}/wayland-protocols)
set(PROTOS
  ext-input-trigger-registration-v1.xml
  ext-input-trigger-action-v1.xml
)

set(GENERATED_HEADERS "")
set(GENERATED_SOURCES "")
foreach(proto IN LISTS PROTOS)
    get_filename_component(name ${proto} NAME_WE)
    set(out_h ${GEN_DIR}/${name}-client-protocol.h)
    set(out_c ${GEN_DIR}/${name}-client-protocol.c)

    # generate the client header
    add_custom_command(
    OUTPUT ${out_h}
    COMMAND ${WAYLAND_SCANNER} client-header ${PROTOCOL_DIR}/${proto} ${out_h}
    DEPENDS ${PROTOCOL_DIR}/${proto}
    COMMENT "Generating ${out_h}"
    VERBATIM
  )

    # generate the private-code (implementation) C file
    add_custom_command(
    OUTPUT ${out_c}
    COMMAND ${WAYLAND_SCANNER} private-code ${PROTOCOL_DIR}/${proto} ${out_c}
    DEPENDS ${PROTOCOL_DIR}/${proto} ${out_h}
    COMMENT "Generating ${out_c}"
    VERBATIM
  )

    list(APPEND GENERATED_HEADERS ${out_h})
    list(APPEND GENERATED_SOURCES ${out_c} ${out_h})
endforeach()

mir_add_wrapped_executable(trigger_client main.cpp ${GENERATED_SOURCES})

# make the example use the generated headers and link the generated code
target_include_directories(trigger_client PRIVATE ${GEN_DIR} ${CMAKE_BINARY_DIR}/src)
target_link_libraries(trigger_client PRIVATE PkgConfig::XKBCOMMON PkgConfig::WAYLAND_CLIENT)
