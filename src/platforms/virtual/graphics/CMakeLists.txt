include_directories(
    ${server_common_include_dirs}
)

add_library(
    mirplatformgraphicsvirtualobjects OBJECT

    platform.cpp
    display.cpp
    display_configuration.cpp
)

add_library(
    mirplatformgraphicsvirtualobjects-symbols OBJECT

    graphics.cpp
)

target_link_libraries(mirplatformgraphicsvirtualobjects-symbols
    PUBLIC
    mirplatform
    mircommon
    mircore
)

target_link_libraries(
    mirplatformgraphicsvirtualobjects

    PRIVATE
    mirplatform
)
