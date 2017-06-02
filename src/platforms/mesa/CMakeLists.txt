set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mir-client-platform-mesa.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mir-client-platform-mesa.pc
  @ONLY
)

include_directories(
  ${CMAKE_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/include/
)

add_subdirectory(common/)
add_subdirectory(server/)
add_subdirectory(client/)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/platforms/mesa/mir_toolkit
  DESTINATION "include/mirplatforms"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mir-client-platform-mesa.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
