add_subdirectory(kms/)

add_library(
  mirsharedgbmservercommon-static STATIC

  display_helpers.cpp
  buffer_allocator.cpp
  buffer_allocator.h
)

target_include_directories(
  mirsharedgbmservercommon-static
  PUBLIC
    ${server_common_include_dirs}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
  mirsharedgbmservercommon-static

  server_platform_common
  kms_utils
  mirplatform
)
