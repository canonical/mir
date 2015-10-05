set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mir-renderer-gl-dev.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mir-renderer-gl-dev.pc
  @ONLY
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/renderers/gl/mir
  DESTINATION "include/mirrenderer"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mir-renderer-gl-dev.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/gl
)

ADD_LIBRARY(
  mirrenderergl OBJECT

  program_family.cpp
  renderer.cpp
  renderer_factory.cpp
)
