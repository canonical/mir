set(ICON_NAME ubuntu-logo)

configure_file(miral-shell.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop @ONLY)
configure_file(mir-shell.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/mir-shell.desktop @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
)

add_custom_target(mir-shell ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/mir-shell.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-shell
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-shell
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mir-shell.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/wayland-sessions
)

install(FILES ${ICON_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps
)
