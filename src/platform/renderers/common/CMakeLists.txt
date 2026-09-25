ADD_LIBRARY(mirrenderercommon OBJECT

  gl_output_filter.cpp
  gl_output_filter.h
  gl_program_factory.cpp
  gl_scene_drawing.cpp
)

target_include_directories(
  mirrenderercommon
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
    ${PROJECT_SOURCE_DIR}/src/platform/renderers
)

target_link_libraries(mirrenderercommon
  PUBLIC
    mircore
  PRIVATE
    mirgl
)
