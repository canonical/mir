set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mir-client-platform-mesa-dev.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mir-client-platform-mesa-dev.pc
  @ONLY
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(server/)
add_subdirectory(client/)


install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/mir_toolkit
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mir-client-platform-mesa-dev.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
