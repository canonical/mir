include_directories(
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/server
)

ADD_LIBRARY(
  mirrenderergl OBJECT

  gl_program_family.cpp
  gl_renderer.cpp
  gl_renderer_factory.cpp
)
