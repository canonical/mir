ADD_LIBRARY(mirrendererblitter OBJECT

  software_egl_context.cpp
  software_egl_context.h
  framebuffer_pool.cpp
  framebuffer_pool.h
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
    mirrenderercommon
  PRIVATE
    mirgl
    PkgConfig::EGL
    PkgConfig::GLESv2
)
