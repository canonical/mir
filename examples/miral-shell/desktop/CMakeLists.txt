set(ICON_NAME ubuntu-logo)

configure_file(miral-shell.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miral-shell.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES ${ICON_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
