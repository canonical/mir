include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
        ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/include/
  ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedgbm-static STATIC
  gbm_format_conversions.cpp
)
