add_library(mircore
    fatal.cpp
    geometry/rectangle.cpp
    geometry/rectangles.cpp
    geometry/ostream.cpp
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
    SOVERSION 1
    LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
    LINK_DEPENDS ${symbol_map}
)
