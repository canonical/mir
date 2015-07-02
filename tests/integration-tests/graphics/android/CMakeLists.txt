list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_hardware_sanity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/patterns.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_graphics_region_factory.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsandroidobjects>
)

#include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
#add_definitions(-DANDROID)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
