add_library(
  mirsharedandroid STATIC

  mir_native_window.cpp
  syncfence.cpp
)

set(MIR_COMMON_PLATFORM_LIBRARIES mirsharedandroid PARENT_SCOPE)
