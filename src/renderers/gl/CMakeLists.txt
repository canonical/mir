configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mir-renderer-gl-dev.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mir-renderer-gl-dev.pc
  @ONLY
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/renderers/gl/mir
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mirrenderer"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mir-renderer-gl-dev.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

ADD_LIBRARY(
  mirrenderergl OBJECT

  renderer.cpp
  renderer_factory.cpp
)

target_include_directories(
  mirrenderergl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/renderer
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/platform
    ${PROJECT_SOURCE_DIR}/src/include/server
    ${PROJECT_SOURCE_DIR}/src/include/gl
)

target_link_libraries(mirrenderergl
  PUBLIC
    mirplatform
    mircommon
    mircore
)
