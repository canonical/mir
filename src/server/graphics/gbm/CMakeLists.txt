include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

# gbm.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


add_library(
  mirplatformgraphics STATIC

  gbm_platform.cpp
  gbm_buffer_allocator.cpp
  gbm_buffer.cpp
  gbm_display_helpers.cpp
  gbm_display.cpp
  kms_display_configuration.cpp
  drm_mode_resources.cpp
  gbm_display_buffer.cpp
  kms_output.cpp
  kms_output_container.cpp
  kms_page_flipper.cpp
)

target_link_libraries(
  mirplatformgraphics
  mircompositor
  mirlogging

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
