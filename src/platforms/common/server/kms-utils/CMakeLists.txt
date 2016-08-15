include_directories(
  ${DRM_INCLUDE_DIRS}
)

add_library(kms_utils STATIC
  drm_mode_resources.cpp
  drm_mode_resources.h
  kms_connector.cpp
  kms_connector.h
)

target_link_libraries(kms_utils
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)
