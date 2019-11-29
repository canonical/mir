mir_add_wrapped_executable(miral-system-compositor
        system_compositor_main.cpp
)

target_include_directories(miral-system-compositor
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/client
)

target_link_libraries(miral-system-compositor miral)
