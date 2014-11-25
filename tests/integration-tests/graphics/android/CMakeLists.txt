list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_render.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/patterns.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_graphics_region_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_client.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsandroidobjects>
)

#include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
#add_definitions(-DANDROID)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
