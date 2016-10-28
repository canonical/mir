set(MIRCORE_ABI 1)

add_library(mircore SHARED
    fatal.cpp
    geometry/rectangle.cpp
    geometry/rectangles.cpp
    geometry/ostream.cpp
    ${PROJECT_SOURCE_DIR}/include/core/mir/int_wrapper.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/optional_value.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/fatal.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/length.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/rectangle.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/point.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/rectangles.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/displacement.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/size.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/forward.h
    ${PROJECT_SOURCE_DIR}/include/core/mir/geometry/dimensions.h
    ${PROJECT_SOURCE_DIR}/include/core/mir_toolkit/common.h
    ${PROJECT_SOURCE_DIR}/include/core/mir_toolkit/mir_version_number.h
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(mircore
    PROPERTIES
    SOVERSION ${MIRCORE_ABI}
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mircore.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mircore.pc
    @ONLY
)

install(TARGETS mircore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/core/mir ${CMAKE_SOURCE_DIR}/include/core/mir_toolkit
    DESTINATION "include/mircore"
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/mircore.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
