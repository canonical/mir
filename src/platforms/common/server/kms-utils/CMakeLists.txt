include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/common
)

set(KMS_UTILS_STATIC_LIBRARY "kms_utils")
set(KMS_UTILS_STATIC_LIBRARY ${KMS_UTILS_STATIC_LIBRARY} PARENT_SCOPE)

add_library(${KMS_UTILS_STATIC_LIBRARY} STATIC
  drm_mode_resources.cpp
  drm_mode_resources.h
  kms_connector.cpp
  kms_connector.h
)

target_link_libraries(${KMS_UTILS_STATIC_LIBRARY}
  PkgConfig::DRM
)
