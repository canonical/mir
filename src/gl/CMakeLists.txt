include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

ADD_LIBRARY(
  mirgl OBJECT

  tessellation_helpers.cpp
)


target_link_libraries(mirgl
  PUBLIC
    mircommon
    mircore
)
