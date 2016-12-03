install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/renderer/mir
  DESTINATION "include/mirrenderer"
)

set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirrenderer")
set(PLATFORM_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirplatform")
set(COMMON_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirrenderer.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirrenderer.pc
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirrenderer.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
