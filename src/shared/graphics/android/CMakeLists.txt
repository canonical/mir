add_library(
  mirsharedandroid STATIC

  mir_native_window.cpp
  refcounted_buffer.cpp
  android_native_buffer.cpp
  syncfence.cpp
)

set(MIR_COMMON_PLATFORM_LIBRARIES mirsharedandroid PARENT_SCOPE)
