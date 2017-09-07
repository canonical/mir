set(ICON_NAME ubuntu-logo)
set(PROGRAM_NAME miral-app)

configure_file(miral-shell.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
)

install(FILES ${ICON_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps
)
