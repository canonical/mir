add_library(kms_utils STATIC
  drm_mode_resources.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/kms/drm_mode_resources.h
  kms_connector.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/kms/kms_connector.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/kms/drm_event_handler.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/kms/threaded_drm_event_handler.h
  threaded_drm_event_handler.cpp
)

target_include_directories(kms_utils
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
)

target_link_libraries(kms_utils
  PkgConfig::DRM
  mircore
  mircommon
)
