get_filename_component(
  PROTOCOL_DIR src/server/frontend_wayland/protocol/
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

get_filename_component(
  GENERATED_DIR src/server/frontend_wayland/generated/
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

set(GENERATED_FILES "")

macro(GENERATE_PROTOCOL NAME_PREFIX PROTOCOL_NAME)
  set(PROTOCOL_PATH "${PROTOCOL_DIR}/${PROTOCOL_NAME}.xml")
  set(OUTPUT_PATH_C "${GENERATED_DIR}/${PROTOCOL_NAME}.c")
  set(OUTPUT_PATH_H "${GENERATED_DIR}/${PROTOCOL_NAME}.h")
  set(OUTPUT_PATH_WRAPPER "${GENERATED_DIR}/${PROTOCOL_NAME}_wrapper.h")
  add_custom_command(OUTPUT "${OUTPUT_PATH_C}"
    VERBATIM
    COMMAND "wayland-scanner" "--include-core-only" "code" "${PROTOCOL_PATH}" "${OUTPUT_PATH_C}"
    DEPENDS "${PROTOCOL_PATH}"
  )
  add_custom_command(OUTPUT "${OUTPUT_PATH_H}"
    VERBATIM
    COMMAND "wayland-scanner" "--include-core-only" "server-header" "${PROTOCOL_PATH}" "${OUTPUT_PATH_H}"
    DEPENDS "${PROTOCOL_PATH}"
  )
  add_custom_command(OUTPUT "${OUTPUT_PATH_WRAPPER}"
    VERBATIM
    COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wayland-wrapper-generator ${NAME_PREFIX} ${PROTOCOL_NAME}.h ${PROTOCOL_PATH} > ${OUTPUT_PATH_WRAPPER}"
    DEPENDS "${PROTOCOL_PATH}"
  )
  set(GENERATED_FILES ${GENERATED_FILES} "${OUTPUT_PATH_H}" "${OUTPUT_PATH_C}" "${OUTPUT_PATH_WRAPPER}")
endmacro()

# when adding a protocol, don't forget to add the generated .c file to CMake
GENERATE_PROTOCOL("wl_" "wayland")
GENERATE_PROTOCOL("z" "xdg-shell-unstable-v6")
GENERATE_PROTOCOL("_" "xdg-shell") # empty prefix is not allowed, but '_' won't match anything, so it is ignored

add_custom_target(refresh-wayland-wrapper
  DEPENDS ${GENERATED_FILES}
  DEPENDS wayland-wrapper-generator
  SOURCES ${GENERATED_FILES}
)
