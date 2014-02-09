add_subdirectory(logging)
add_subdirectory(lttng)
add_subdirectory(null)

add_library(
    mirreport STATIC
    default_server_configuration.cpp
    )

target_link_libraries(mirreport mirserverlttng mirlogging mirnullreport)
