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
)

target_link_libraries(miral-shell
    example-shell-lib
)
