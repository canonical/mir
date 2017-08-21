include_directories(
        ${PROJECT_SOURCE_DIR}/include/miral
        ${PROJECT_SOURCE_DIR}/include/client
)

add_subdirectory(spinner)
add_subdirectory(desktop)

add_custom_target(miral-run ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-run.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-xrun ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-xrun.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-screencast ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-screencast.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-desktop ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-desktop.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-app ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-app.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

mir_add_wrapped_executable(miral-shell
    shell_main.cpp
    tiling_window_manager.cpp   tiling_window_manager.h
        floating_window_manager.cpp floating_window_manager.h
    decoration_provider.cpp     decoration_provider.h
    titlebar_config.cpp         titlebar_config.h
)

pkg_check_modules(FREETYPE freetype2 REQUIRED)
target_include_directories(miral-shell PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(miral-shell PRIVATE -DTYPO_SUPPORTS_FREETYPE)
target_link_libraries(miral-shell
    miral-spinner
    miral
    ${FREETYPE_LIBRARIES}
)

