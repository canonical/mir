list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test-dimensions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-dimensions-generic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-size.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-point.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-displacement.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-rectangle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test-rectangles.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
