add_subdirectory(desktop)
add_subdirectory(spinner)

add_custom_target(miral-app ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-app.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_target(miral-terminal ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-terminal.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-terminal
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-terminal
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

mir_add_wrapped_executable(miral-shell
    shell_main.cpp
    wasm_window_manager.cpp
)
set(WASMER_INCLUDE_DIR "/usr/local/include")
set(WASMER_LIBRARY "/usr/local/lib/libwasmer.so")

target_include_directories(miral-shell PRIVATE ${WASMER_INCLUDE_DIR})

target_link_libraries(miral-shell
    PRIVATE
    example-shell-lib
    miral-spinner
    ${WASMER_LIBRARY}
)
