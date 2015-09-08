file(GLOB UDEV_FILES *.umockdev *.ioctl *.evemu)

install(FILES ${UDEV_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/udev_recordings)
