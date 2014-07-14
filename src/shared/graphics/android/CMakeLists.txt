include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})

add_definitions( -DANDROID )

add_library(
  mirsharedandroid STATIC

  mir_native_window.cpp
  refcounted_buffer.cpp
  android_native_buffer.cpp
  syncfence.cpp
)

list(APPEND MIR_COMMON_LIBRARIES
  mirsharedandroid
)

set(MIR_COMMON_LIBRARIES ${MIR_COMMON_LIBRARIES} PARENT_SCOPE)
