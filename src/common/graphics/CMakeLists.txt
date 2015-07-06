if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android)
endif()

add_library(mircommongraphics OBJECT
  pixel_formats.cpp
)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mircommongraphics>
)

set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)

