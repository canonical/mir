ADD_LIBRARY(mirrenderergl OBJECT

  renderer.cpp
  renderer_factory.cpp
)

target_include_directories(
  mirrenderergl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
)

target_include_directories(mirrenderergl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirrenderergl
  PUBLIC
    mircore
  PRIVATE
    mirgl
)
