pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})

add_executable(wrapper-generator
  wrapper_generator.cpp
  utils.cpp                 utils.h
  enum.cpp                  enum.h
  argument.cpp              argument.h
  method.cpp                method.h
  request.cpp               request.h
  event.cpp                 event.h
  interface.cpp             interface.h
  emitter.cpp               emitter.h
)

target_link_libraries(wrapper-generator
  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)

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

add_custom_target(refresh-wayland-wrapper
  DEPENDS wrapper-generator
)

macro(GENERATE_PROTOCOL NAME_PREFIX PROTOCOL_NAME)
  set(PROTOCOL_PATH "${PROTOCOL_DIR}/${PROTOCOL_NAME}.xml")
  set(OUTPUT_PATH_HEADER "${GENERATED_DIR}/${PROTOCOL_NAME}_wrapper.h")
  set(OUTPUT_PATH_SRC "${GENERATED_DIR}/${PROTOCOL_NAME}_wrapper.cpp")
  set(PROTOCOL_TARGET "${PROTOCOL_NAME}-protocol")
  add_custom_target("${PROTOCOL_TARGET}"
    DEPENDS "${PROTOCOL_PATH}"
    DEPENDS wrapper-generator
    SOURCES "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}"
    VERBATIM
    COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator ${NAME_PREFIX} ${PROTOCOL_PATH} header > ${OUTPUT_PATH_HEADER}"
    COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator ${NAME_PREFIX} ${PROTOCOL_PATH} source > ${OUTPUT_PATH_SRC}"
    COMMAND "printf" "\\x1b[34;1mGenerated wrapper files for ${PROTOCOL_NAME} protocol\\x1b[0m\\n"
  )
  add_dependencies(refresh-wayland-wrapper "${PROTOCOL_TARGET}")
endmacro()

# when adding a protocol, don't forget to add the generated .c file to CMake
GENERATE_PROTOCOL("wl_" "wayland")
GENERATE_PROTOCOL("z" "xdg-shell-unstable-v6")
GENERATE_PROTOCOL("_" "xdg-shell") # empty prefix is not allowed, but '_' won't match anything, so it is ignored
GENERATE_PROTOCOL("zwlr_" "wlr-layer-shell-unstable-v1")


