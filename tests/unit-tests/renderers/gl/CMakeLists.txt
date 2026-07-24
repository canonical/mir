list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_renderer.cpp
  ${PROJECT_SOURCE_DIR}/src/platform/renderers/gl/renderer_factory.cpp
  ${PROJECT_SOURCE_DIR}/src/platform/renderers/gl/renderer_logger.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
