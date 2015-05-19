include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})

add_definitions( -DANDROID )

add_library(
  mirsharedandroid-static STATIC

  mir_native_window.cpp
  refcounted_buffer.cpp
  android_native_buffer.cpp
  syncfence.cpp
)
