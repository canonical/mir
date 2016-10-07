include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/include/
  ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedmesa-static STATIC
  gbm_format_conversions.cpp
)
