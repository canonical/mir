include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/3rd_party/android-deps)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include/platform)

add_definitions( -DANDROID )

add_library(
  mirsharedandroid-static STATIC

  mir_native_window.cpp
  refcounted_buffer.cpp
  android_native_buffer.cpp
  syncfence.cpp
)
