set(PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/server-decoration.xml")
set(OUTPUT_PATH_HEADER "${CMAKE_CURRENT_BINARY_DIR}/server-decoration_wrapper.h")
set(OUTPUT_PATH_SRC "${CMAKE_CURRENT_BINARY_DIR}/server-decoration_wrapper.cpp")

add_custom_command(
    OUTPUT "${OUTPUT_PATH_SRC}" "${OUTPUT_PATH_HEADER}"
    COMMAND "sh" "-c"
    "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator org_kde_kwin_ ${PROTOCOL} header >${OUTPUT_PATH_HEADER}"
    COMMAND "sh" "-c"
    "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator org_kde_kwin_ ${PROTOCOL} source >${OUTPUT_PATH_SRC}"
    DEPENDS mir_wayland_generator ${PROTOCOL}
)

add_custom_target(refresh-test-wayland-wrapper
    DEPENDS mir_wayland_generator "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}"
    SOURCES "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}"
)

set(GENERATED_FILES "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}" PARENT_SCOPE)
