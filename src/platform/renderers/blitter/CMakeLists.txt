ADD_LIBRARY(mirrendererblitter OBJECT

  egl_context.cpp
  egl_context.h
  framebuffer_pool.cpp
  framebuffer_pool.h
  texture_cache.cpp
  texture_cache.h
)

target_include_directories(
  mirrendererblitter
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/platform
)

target_link_libraries(mirrendererblitter
  PUBLIC
    mircore
  PRIVATE
    mirgl
    mirrenderercommon
    PkgConfig::EGL
    PkgConfig::GLESv2
)
