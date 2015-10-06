include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

ADD_LIBRARY(
  mirgl OBJECT

  default_program_factory.cpp
  program.cpp
  recently_used_cache.cpp
  tessellation_helpers.cpp
  texture.cpp
)
