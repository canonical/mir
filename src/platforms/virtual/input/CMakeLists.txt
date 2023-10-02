include_directories(
    ${server_common_include_dirs}
)

add_library(
    mirplatforminputvirtualobjects OBJECT

    input_platform.cpp
)

target_link_libraries(mirplatforminputvirtualobjects
    PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(
    mirplatforminputvirtualobjects-symbols OBJECT

    input.cpp
)

target_link_libraries(mirplatforminputvirtualobjects-symbols
    PUBLIC
    mirplatform
    mircommon
)
