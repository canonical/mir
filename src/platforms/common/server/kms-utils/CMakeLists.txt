add_library(kms_utils STATIC
  drm_mode_resources.cpp
  drm_mode_resources.h
  kms_connector.cpp
  kms_connector.h
  drm_event_handler.h
  threaded_drm_event_handler.h
  threaded_drm_event_handler.cpp
)

target_include_directories(kms_utils
  PUBLIC ${server_common_include_dirs}
)

target_link_libraries(kms_utils
  PkgConfig::DRM
  mircore
  mircommon
)
