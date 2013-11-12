include_directories(${GLESv2_INCLUDE_DIRS})

add_subdirectory(nested/)

if (MIR_PLATFORM STREQUAL "android")
  include_directories(${LIBHARDWARE_INCLUDE_DIRS})
  add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(gbm/)
endif()

