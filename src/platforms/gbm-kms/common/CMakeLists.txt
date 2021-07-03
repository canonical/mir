include_directories(
  ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/include/
  ${DRM_INCLUDE_DIRS}
)

add_library(
  mirsharedgbm-static STATIC
  gbm_format_conversions.cpp
)

target_link_libraries(mirsharedgbm-static
  PUBLIC
    mircore
)
