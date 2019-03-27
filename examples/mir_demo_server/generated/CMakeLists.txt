set(PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/server-decoration.xml")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.h
    DEPENDS mir_wayland_generator
    DEPENDS ${PROTOCOL}
    COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator org_kde_kwin_ ${PROTOCOL} header >${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.h"
    COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator org_kde_kwin_ ${PROTOCOL} source >${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.cpp"
)

set(PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/xdg-decoration-unstable-v1.xml")

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.cpp
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.h
        DEPENDS mir_wayland_generator
        DEPENDS ${PROTOCOL}
        COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator zxdg_ ${PROTOCOL} header >${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.h"
        COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/mir_wayland_generator zxdg_ ${PROTOCOL} source >${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.cpp"
)

add_custom_target(refresh-demo-wayland-wrapper
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.h
    DEPENDS mir_wayland_generator
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server-decoration_wrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration-unstable-v1_wrapper.h
)

#xdg-decoration-unstable-v1.xml

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)
