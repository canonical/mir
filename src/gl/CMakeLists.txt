include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/gl
)

ADD_LIBRARY(
  mirgl OBJECT

  tessellation_helpers.cpp
)


target_link_libraries(mirgl
  PUBLIC
    mirplatform
    mircommon
    mircore
)
