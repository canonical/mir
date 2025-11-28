ADD_LIBRARY(mirrenderergl OBJECT

  renderer.cpp
  renderer_factory.cpp
)

target_include_directories(
  mirrenderergl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
)

target_link_libraries(mirrenderergl
  PUBLIC
    mircore
  PRIVATE
    mirgl
)
