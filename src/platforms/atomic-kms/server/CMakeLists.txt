add_subdirectory(kms/)

add_library(
  mirsharedatomickmscommon-static STATIC

  display_helpers.cpp
  gbm_display_allocator.h
  gbm_display_allocator.cpp
)

target_include_directories(
  mirsharedatomickmscommon-static
  PUBLIC
    ${server_common_include_dirs}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
  mirsharedatomickmscommon-static

  server_platform_common
  kms_utils
  mirplatform
)
