add_library(mirgl OBJECT

  tessellation_helpers.cpp
)

target_include_directories(mirgl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
    ${PROJECT_SOURCE_DIR}/src/include/gl
)

target_link_libraries(mirgl
  PUBLIC
    mircommon
    mircore
)
