if(MIR_TEST_PLATFORM STREQUAL "mesa-kms")
list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
)
elseif(MIR_TEST_PLATFORM STREQUAL "mesa-x11")
list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/x11/X11_resources.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
)
endif()

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE
)
